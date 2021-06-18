/* AUTORIGHTS
Copyright (C) 2007 Princeton University

This file is part of Ferret Toolkit.

Ferret Toolkit is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

FastFlow version by Daniele De Sensi (d.desensi.software@gmail.com)

This version is like ferret-ff-farm.cpp but part of the processing performed
by the emitter (i.e. file loading) is moved to the farm's workers.

*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <cass.h>

#undef fatal // Defined in cass, will collide with variables named as 'fatal', e.g. "bool fatal = true;"

#include <cass_timer.h>
#include <../image/image.h>
#include "tpool.h"

#ifdef ENABLE_NORNIR
#include <nornir/instrumenter.hpp>
#include <stdlib.h>
#include <iostream>
std::string getParametersPath(){
    return std::string(getenv("PARSECDIR")) + std::string("/parameters.xml");
}
nornir::Instrumenter* instr;
#endif //ENABLE_NORNIR

#define NO_DEFAULT_MAPPING
#define ENABLE_FF_ONDEMAND

#include <ff/dnode.hpp>
#include <ff/node.hpp>
#include <ff/svector.hpp>
#include <ff/pipeline.hpp>
#include <ff/d/inter.hpp>
#include <ff/d/zmqTransport.hpp>
#include <ff/d/zmqImpl.hpp>

using namespace ff;

#define COMM1   zmqOnDemand
#define COMM2   zmqFromAny

unsigned taskSize=0;
// the following SPSC unbounded queue is shared between the Worker and the Sender threads
// to free unused memory
uSWSR_Ptr_Buffer RECYCLE(1024); // NOTE: See also the farm_farm test for another solution to the same problem !

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#define DEFAULT_DEPTH    25
#define MAXR    100
#define IMAGE_DIM    14

const char *db_dir = NULL;
const char *table_name = NULL;
const char *query_dir = NULL;
const char *output_path = NULL;
size_t nthreads;

FILE *fout;

int DEPTH = DEFAULT_DEPTH;

int top_K = 10;

char *extra_params = "-L 8 - T 20";

cass_env_t *env;
cass_table_t *table;
cass_table_t *query_table;

int vec_dist_id = 0;
int vecset_dist_id = 0;

struct load_data {
    int width, height;
    char *name;
    unsigned char *HSV, *RGB;
};

struct seg_data {
    int width, height, nrgn;
    char *name;
    unsigned char *mask;
    unsigned char *HSV;
};

struct extract_data {
    cass_dataset_t ds;
    char *name;
};

struct vec_query_data {
    char *name;
    cass_dataset_t *ds;
    cass_result_t result;
};

struct rank_data {
    char *name;
    cass_dataset_t *ds;
    cass_result_t result;
};

/* ------- The Helper Functions ------- */
int cnt_enqueue;
int cnt_dequeue;


/* ------ The Stages ------ */
class Load : public ff_dnode<COMM1> {
    typedef COMM1::TransportImpl transport_t;

private:
    unsigned nTasks;
    unsigned nHosts;

    char path[BUFSIZ];

    int scan_dir(const char *, char *head);

    int dir_helper(char *dir, char *head) {
        DIR *pd = NULL;
        struct dirent *ent = NULL;
        int result = 0;
        pd = opendir(dir);
        if (pd == NULL) goto except;
        for (;;) {
            ent = readdir(pd);
            if (ent == NULL) break;
            if (scan_dir(ent->d_name, head) != 0) return -1;
        }
        goto final;

        except:
        result = -1;
        perror("Error:");
        final:
        if (pd != NULL) closedir(pd);
        return result;
    }

public:
    Load(unsigned nTasks, unsigned nHosts, const std::string& name, const std::string& address, transport_t* const transp):
            nTasks(nTasks),nHosts(nHosts),name(name),address(address),transp(transp) {
    }

    int svc_init() {
        // the callback will be called as soon as the output message is no
        // longer in use by the transport layer
        struct timeval start, stop;
        gettimeofday(&start, NULL);
        ff_dnode<COMM1>::init(name, address, nHosts, transp, SENDER, 0, callback);

        gettimeofday(&stop, NULL);
        fprintf(stderr,"Emitter init time %f ms\n", diffmsec(stop, start));
        return 0;
    }

    void *svc(void *task) {
        fprintf(stderr,"Load : received task\n");
        if (--nTasks == 0) {
            ff_send_out(task);
            fprintf(stderr,"Load : sent task1\n");
            return EOS; // generates EOS
        }
        fprintf(stderr,"Load : sent task2\n");
        return task;
//        fprintf(stderr,"emitter 1 received task\n");
//        path[0] = 0;
//        if (strcmp(query_dir, ".") == 0) {
//            dir_helper(".", path);
//        } else {
//            scan_dir(query_dir, path);
//        }
//        fprintf(stderr,"emitter 1 returns EOS\n");
//        return EOS;
    }

    void prepare(svector<iovec>& v, void* ptr, const int sender=-1) {
        fprintf(stderr,"Load : prepare \n");
        struct iovec iov={ptr,taskSize*taskSize*sizeof(double)};
        v.push_back(iov);
    }

protected:
    const std::string name;
    const std::string address;
    transport_t *transp;

    static void callback(void *e, void *arg) {
        fprintf(stderr,"emittter 1 callback\n");
        delete[] (double *) e;
    }
};

int Load::scan_dir(const char *dir, char *head) {
    struct stat st;
    int ret;
    /* test for . and .. */
    if (dir[0] == '.') {
        if (dir[1] == 0) return 0;
        else if (dir[1] == '.') {
            if (dir[2] == 0) return 0;
        }
    }

    /* append the name to the path */
    strcat(head, dir);
    ret = stat(path, &st);
    if (ret != 0) {
        perror("Error:");
        return -1;
    }
    if (S_ISREG(st.st_mode)) {
        char *pathtask = new char[BUFSIZ];
        pathtask = strdup(path);
        ff_send_out((void *) pathtask);
        cnt_enqueue++;
    } else if (S_ISDIR(st.st_mode)) {
        strcat(head, "/");
        dir_helper(path, head + strlen(head));
    }
    /* removed the appended part */
    head[0] = 0;
    return 0;
}

/* the whole path to the file */
struct load_data *file_helper(const char *file) {
    int r;
    struct load_data *data;

    data = (struct load_data *) malloc(sizeof(struct load_data));
    assert(data != NULL);

    data->name = strdup(file);

    r = image_read_rgb_hsv(file, &data->width, &data->height, &data->RGB, &data->HSV);
    assert(r == 0);
    return data;
}

struct seg_data *segment(char *filename) {
    struct load_data *load = file_helper(filename);
    delete[] filename;
    assert(load != NULL);
    struct seg_data *seg = (struct seg_data *) calloc(1, sizeof(struct seg_data));

    seg->name = load->name;

    seg->width = load->width;
    seg->height = load->height;
    seg->HSV = load->HSV;
    image_segment(&seg->mask, &seg->nrgn, load->RGB, load->width, load->height);

    free(load->RGB);
    free(load);

    return seg;
}

struct extract_data *extract(struct seg_data *seg) {
    assert(seg != NULL);
    struct extract_data *extract = (struct extract_data *) calloc(1, sizeof(struct extract_data));

    extract->name = seg->name;

    image_extract_helper(seg->HSV, seg->mask, seg->width, seg->height, seg->nrgn, &extract->ds);

    free(seg->mask);
    free(seg->HSV);
    free(seg);

    return extract;
}

struct vec_query_data *vec(struct extract_data *extract) {
    assert(extract != NULL);
    struct vec_query_data *vec = (struct vec_query_data *) calloc(1, sizeof(struct vec_query_data));
    vec->name = extract->name;

    cass_query_t query;
    memset(&query, 0, sizeof query);
    query.flags = CASS_RESULT_LISTS | CASS_RESULT_USERMEM;

    vec->ds = query.dataset = &extract->ds;
    query.vecset_id = 0;

    query.vec_dist_id = vec_dist_id;

    query.vecset_dist_id = vecset_dist_id;

    query.topk = 2 * top_K;

    query.extra_params = extra_params;

    cass_result_alloc_list(&vec->result, vec->ds->vecset[0].num_regions, query.topk);

//	cass_result_alloc_list(&result_m, 0, T1);
//	cass_table_query(table, &query, &vec->result);
    cass_table_query(table, &query, &vec->result);

    return vec;
}

struct rank_data *rank(struct vec_query_data *vec) {
    cass_query_t query;
    assert(vec != NULL);

    struct rank_data *rank = (struct rank_data *) calloc(1, sizeof(struct rank_data));
    rank->name = vec->name;

    query.flags = CASS_RESULT_LIST | CASS_RESULT_USERMEM | CASS_RESULT_SORT;
    query.dataset = vec->ds;
    query.vecset_id = 0;

    query.vec_dist_id = vec_dist_id;

    query.vecset_dist_id = vecset_dist_id;

    query.topk = top_K;

    query.extra_params = NULL;

    cass_result_t *candidate = cass_result_merge_lists(&vec->result, (cass_dataset_t *) query_table->__private, 0);
    query.candidate = candidate;


    cass_result_alloc_list(&rank->result, 0, top_K);
    cass_table_query(query_table, &query, &rank->result);

    cass_result_free(&vec->result);
    cass_result_free(candidate);
    free(candidate);
    cass_dataset_release(vec->ds);
    free(vec->ds);
    free(vec);
    return rank;
}




class Out : public ff_dnode<COMM2> {
    typedef COMM2::TransportImpl transport_t;
public:
    Out(unsigned nTasks, unsigned nHosts, const std::string& name, const std::string& address, transport_t* const transp):
            nTasks(nTasks),nHosts(nHosts),name(name),address(address),transp(transp) {
    }

    // initializes dnode
    int svc_init() {
        struct timeval start, stop;
        gettimeofday(&start, NULL);
        ff_dnode<COMM2>::init(name, address, nHosts, transp, RECEIVER, 0);
        gettimeofday(&stop, NULL);

        fprintf(stderr,"Collector init time %f ms\n", diffmsec(stop, start));
        return 0;
    }

    void* svc(void* task){
        fprintf(stderr,"Out : received task\n");
        if (task == NULL) {
            srandom(0); //::getpid()+(getusec()%4999));
            for(unsigned i=0;i<nTasks;++i) {
                double* t = new double[taskSize*taskSize];
                assert(t);
                for(unsigned j=0;j<(taskSize*taskSize);++j)
                    t[j] = 1.0*random() / (double)(taskSize);

                ff_send_out((void*)t);
            }
            return GO_ON;
        }

        printf("got result\n");
        return GO_ON;
//        fprintf(stderr,"lol3\n");
//        if(task != NULL) {
//            struct rank_data *rank = (struct rank_data *) task;
//
//            assert(rank != NULL);
//
//            fprintf(fout, "%s", rank->name);
//
//            ARRAY_BEGIN_FOREACH(rank->result.u.list, cass_list_entry_t
//            p)
//            {
//                char *obj = NULL;
//                if (p.dist == HUGE) continue;
//                cass_map_id_to_dataobj(query_table->map, p.id, &obj);
//                assert(obj != NULL);
//                fprintf(fout, "\t%s:%g", obj, p.dist);
//            }
//            ARRAY_END_FOREACH;
//
//            fprintf(fout, "\n");
//
//            cass_result_free(&rank->result);
//            free(rank->name);
//            free(rank);
//
//            cnt_dequeue++;
//
//            fprintf(stderr, "(%d,%d)\n", cnt_enqueue, cnt_dequeue);
//#ifdef ENABLE_NORNIR
//            instr->end();
//#endif //ENABLE_NORNIR
//        }else{
//            fprintf(stderr,"lol2\n");
//            return GO_ON;
//        }
//        printf("lol1\n");
//        return GO_ON;
    }

private:
    unsigned nTasks;
    unsigned nHosts;
protected:
    const std::string name;
    const std::string address;
    transport_t *transp;
    unsigned cnt;
};


struct ff_task_t {
    double* getData() { return (double*)(msg->getData()); }
    COMM1::TransportImpl::msg_t*   msg;
    ff_task_t*  self;
};


class CollapsedPipeline : public ff_dnode<COMM1> {
    typedef COMM1::TransportImpl        transport_t;
public:
    CollapsedPipeline(const std::string& name, const std::string& address, transport_t* const transp):
            name(name),address(address),transp(transp) {
    }

    int svc_init() {
        fprintf(stderr,"collapsed : int\n");
        myt = new double[taskSize*taskSize];
        assert(myt);
        c = new double[taskSize*taskSize];
        assert(c);
        for(unsigned j=0;j<(taskSize*taskSize);++j)
            c[j] = j*1.0;

        ff_dnode<COMM1>::init(name, address, 1, transp, RECEIVER, transp->getProcId());
        return 0;
    }

    void *svc(void *task) {
        fprintf(stderr,"Collapsed : received task\n");
        //fprintf(stderr,"Worker received task\n");
        //return rank(vec(extract(segment((char *) task))));
        static unsigned c=0;
        double* t = (double*)task;
        printf("Worker: get one task %d\n",++c);

        memset(myt,0,taskSize*taskSize*sizeof(double));

        for(unsigned i=0;i<taskSize;++i)
            for(unsigned j=0;j<taskSize;++j)
                for(unsigned k=0;k<taskSize;++k)
                    myt[i*taskSize+j] += t[i*taskSize+k]*t[k*taskSize+j];

        memcpy(t,myt,taskSize*taskSize*sizeof(double));
        return t;
    }

    void svc_end() {
        fprintf(stderr,"Collapsed : end\n");
        delete [] myt;
        delete [] c;
    }

    void prepare(svector<msg_t*>*& v, size_t len, const int sender=-1) {
        fprintf(stderr,"Collapsed : prepare\n");
        svector<msg_t*> * v2 = new svector<msg_t*>(len);
        assert(v2);
        for(size_t i=0;i<len;++i) {
            msg_t * m = new msg_t;
            assert(m);
            v2->push_back(m);
            RECYCLE.push(m);
        }
        v = v2;
    }

private:
    double * myt;
    double * c;
protected:
    const int nw;
    const std::string name;
    const std::string address;
    transport_t *transp;

    unsigned cnt;
};


class Sender: public ff_dnode<COMM2> {
    typedef COMM2::TransportImpl        transport_t;
protected:
    static void callback(void *e,void*) {
        msg_t* p;
        if (!RECYCLE.pop((void**)&p)) abort();
        delete p;
    }
public:
    Sender(const std::string& name, const std::string& address, transport_t* const transp):
            name(name),address(address),transp(transp) {
    }

    int svc_init() {
        fprintf(stderr,"Sender : init\n");
        // the callback will be called as soon as the output message is no
        // longer in use by the transport layer
        ff_dnode<COMM2>::init(name, address, 1, transp, SENDER, transp->getProcId(),callback);
        return 0;
    }

    void* svc(void* task) {
        fprintf(stderr,"Sender, sending the task\n");
        return task;
    }

    void prepare(svector<iovec>& v, void* ptr, const int sender=-1) {
        fprintf(stderr,"Sender: preapre\n");
        struct iovec iov={ptr,taskSize*taskSize*sizeof(double)};
        v.push_back(iov);
    }
protected:
    const std::string name;
    const std::string address;
    transport_t   * transp;
};

class Emitter2 : public ff_dnode<COMM1> {
    typedef COMM1::TransportImpl transport_t;
public:
    Emitter2(const std::string &name, const std::string &address, transport_t *const transp) :
            name(name), address(address), transp(transp) {
    }

    int svc_init() {
        ff_dnode<COMM1>::init(name, address, 1, transp, RECEIVER, transp->getProcId());
        fprintf(stderr,"emitter 2 init\n");
        return 0;
    }

    void *svc(void *task) {
        fprintf(stderr,"emitter 2 received task\n");
        return task;
    }

    virtual void unmarshalling(svector<msg_t*>* const v[], const int vlen, void *& task) {
        fprintf(stderr,"emitter 2 unmarshalling\n");
        assert(vlen==1 && v[0]->size()==1);
        ff_task_t* t = new ff_task_t;
        t->msg = v[0]->operator[](0);
        t->self= t;
        task = t;
        //task = v[0]->operator[](0)->getData();
        delete v[0];
    }

protected:
    static void callback(void *e, void* arg) {
        fprintf(stderr,"emitter 2 callback\n");
        ff_task_t* t = (ff_task_t*)arg;
        assert(t);
        delete (t->msg);
        delete (t->self);
    }

    const std::string name;
    const std::string address;
    transport_t *transp;
};

int main(int argc, char *argv[]) {
    stimer_t tmr;
    int ret, i;

#ifdef PARSEC_VERSION
    #define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
    printf("PARSEC Benchmark Suite Version " __PARSEC_XSTRING(PARSEC_VERSION) "\n");
    fflush(NULL);
#else
    printf("PARSEC Benchmark Suite\n");
    fflush(NULL);
#endif //PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_begin(__parsec_ferret);
#endif

    if (argc < 8) {
        printf("%s <database> <table> <query dir> <top K> <depth> <n> <out>\n", argv[0]);
        return 0;
    }

    taskSize = 10;
    unsigned numTasks=10;
    char*    P = argv[8];                 // 1 for the master  0 for other hosts
    unsigned nhosts = atoi(argv[6]) - 1;
    char *address1 = "127.0.0.1:8080";
    char *address2 = "127.0.0.1:8081";

    // creates the network using 0mq as transport layer
    zmqTransport transport(nhosts);
    if (transport.initTransport()<0) abort();

    if (atoi(P)) {
        ff_pipeline pipe;
        Out C(numTasks, nhosts, "B", address2, &transport);
        Load   E(numTasks, nhosts, "A", address1, &transport);

        C.skipfirstpop(true);

        pipe.add_stage(&C);
        pipe.add_stage(&E);

        if (pipe.run_and_wait_end()<0) {
            error("running pipeline\n");
            return -1;
        }
        printf("Time= %f\n", pipe.ffwTime());
    } else {
        ff_pipeline pipe;
        CollapsedPipeline W("A", address1, &transport);
        Sender S("B", address2, &transport);
        pipe.add_stage(&W);
        pipe.add_stage(&S);

        if (!RECYCLE.init()) abort();

        if (pipe.run_and_wait_end()<0) {
            error("running pipeline\n");
            return -1;
        }
    }

    transport.closeTransport();
    std::cout << "done\n";

#ifdef ENABLE_NORNIR
    instr = new nornir::Instrumenter(getParametersPath(), 1, NULL, true);
#endif //ENABLE_NORNIR

//    db_dir = argv[1];
//    table_name = argv[2];
//    query_dir = argv[3];
//    top_K = atoi(argv[4]);
//    DEPTH = atoi(argv[5]);
//    nthreads = atoi(argv[6]);
//    output_path = argv[7];
//    fout = fopen(output_path, "w");
//    assert(fout != NULL);
//    cass_init();
//
//    ret = cass_env_open(&env, db_dir, 0);
//    if (ret != 0) {
//        printf("ERROR: %s\n", cass_strerror(ret));
//        return 0;
//    }
//
//    vec_dist_id = cass_reg_lookup(&env->vec_dist, "L2_float");
//    assert(vec_dist_id >= 0);
//
//    vecset_dist_id = cass_reg_lookup(&env->vecset_dist, "emd");
//    assert(vecset_dist_id >= 0);
//
//    i = cass_reg_lookup(&env->table, table_name);
//    table = query_table = cass_reg_get(&env->table, i);
//
//    i = table->parent_id;
//
//    if (i >= 0) {
//        query_table = cass_reg_get(&env->table, i);
//    }
//
//    if (query_table != table) cass_table_load(query_table);
//    cass_map_load(query_table->map);
//    cass_table_load(table);
//    image_init(argv[0]);
//    stimer_tick(&tmr);
//    cnt_enqueue = cnt_dequeue = 0;
//
//#ifdef ENABLE_PARSEC_HOOKS
//    __parsec_roi_begin();
//#endif
//    char *P = argv[8];
//    unsigned nhosts = 1;
//    unsigned nw = 1;
//    char *address1 = "127.0.0.1:8080";
//    char *address2 = "127.0.0.1:8081";
//
//    fprintf(stderr,"Starting\n");
//
//    // creates the network using 0mq as transport layer
//    zmqTransport transport(atoi(P) ? -1 : nhosts);
//    if (transport.initTransport() < 0) abort();
//    unsigned numTasks = 10;
//    if (atoi(P)) {
//        ff_pipeline pipe;
//        Out C(numTasks, nhosts, "B", address2, &transport);
//        Load E(numTasks, nhosts, "A", address1, &transport);
//
//        C.skipfirstpop(true);
//
//        pipe.add_stage(&C);
//        pipe.add_stage(&E);
//
//        fprintf(stderr,"Starting pipeline\n");
//
//        if (pipe.run_and_wait_end() < 0) {
//            error("running pipeline\n");
//            return -1;
//        }
//    } else {
//        fprintf(stderr,"lol4\n");
//        ff_farm<> farm;
//        std::vector < ff::ff_node * > workers;
//        Emitter2 E2("A", address1, &transport);
//        farm.add_emitter(&E2);
//        for (size_t i = 0; i < nw; i++) {
//            workers.push_back(new CollapsedPipeline(nw, "B", address2, &transport));
//        }
//        fprintf(stderr,"lol5\n");
//        farm.add_workers(workers);
//        fprintf(stderr,"lol6\n");
//        if (farm.run_and_wait_end()<0) {
//            error("running farm\n");
//            return -1;
//        }
//    }
//
//#ifdef ENABLE_PARSEC_HOOKS
//    __parsec_roi_end();
//#endif
//
//
//    stimer_tuck(&tmr, "QUERY TIME");
//
//    ret = cass_env_close(env, 0);
//    if (ret != 0) {
//        printf("ERROR: %s\n", cass_strerror(ret));
//        return 0;
//    }
//
//    cass_cleanup();
//
//    image_cleanup();
//
//    fclose(fout);
//
//#ifdef ENABLE_PARSEC_HOOKS
//    __parsec_bench_end();
//#endif
    return 0;
}

