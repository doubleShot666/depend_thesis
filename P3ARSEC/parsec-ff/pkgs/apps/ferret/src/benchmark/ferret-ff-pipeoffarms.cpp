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
#include <cass_timer.h>
#include <../image/image.h>
#include "tpool.h"

#define NO_DEFAULT_MAPPING
#define ENABLE_FF_ONDEMAND
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <iostream>

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#define DEFAULT_DEPTH	25
#define MAXR	100
#define IMAGE_DIM	14

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

struct load_data
{
    int width, height;
    char *name;
    unsigned char *HSV, *RGB;
};

struct seg_data
{
    int width, height, nrgn;
    char *name;
    unsigned char *mask;
    unsigned char *HSV;
};

struct extract_data
{
    cass_dataset_t ds;
    char *name;
};

struct vec_query_data
{
    char *name;
    cass_dataset_t *ds;
    cass_result_t result;
};

struct rank_data
{
    char *name;
    cass_dataset_t *ds;
    cass_result_t result;
};

/* ------- The Helper Functions ------- */
int cnt_enqueue;
int cnt_dequeue;
char path[BUFSIZ];


/* ------ The Stages ------ */
class Load: public ff::ff_node{
private:
    int scan_dir (const char *, char *head);
    int dir_helper (char *dir, char *head)
    {
        DIR *pd = NULL;
        struct dirent *ent = NULL;
        int result = 0;
        pd = opendir(dir);
        if (pd == NULL) goto except;
        for (;;)
        {
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

    /* the whole path to the file */
    int file_helper (const char *file)
    {
        int r;
        struct load_data *data;

        data = (struct load_data *)malloc(sizeof(struct load_data));
        assert(data != NULL);

        data->name = strdup(file);

        r = image_read_rgb_hsv(file, &data->width, &data->height, &data->RGB, &data->HSV);
        assert(r == 0);

        /*
            r = image_read_rgb(file, &data->width, &data->height, &data->RGB);
            r = image_read_hsv(file, &data->width, &data->height, &data->HSV);
            */

        cnt_enqueue++;
        ff_send_out((void*) data);
        return 0;
    }

public:
    Load(){
        path[0] = 0;
    }

    void* svc(void* task){
        if (strcmp(query_dir, ".") == 0){
            dir_helper(".", path);
        }else{
            scan_dir(query_dir, path);
        }

        return EOS;
    }
};

int Load::scan_dir (const char *dir, char *head)
{
    struct stat st;
    int ret;
    /* test for . and .. */
    if (dir[0] == '.')
    {
        if (dir[1] == 0) return 0;
        else if (dir[1] == '.')
        {
            if (dir[2] == 0) return 0;
        }
    }

    /* append the name to the path */
    strcat(head, dir);
    ret = stat(path, &st);
    if (ret != 0)
    {
        perror("Error:");
        return -1;
    }
    if (S_ISREG(st.st_mode)) file_helper(path);
    else if (S_ISDIR(st.st_mode))
    {
        strcat(head, "/");
        dir_helper(path, head + strlen(head));
    }
    /* removed the appended part */
    head[0] = 0;
    return 0;
}


class Seg: public ff::ff_node{
private:
    struct seg_data *seg;
    struct load_data *load;
public:
    void* svc(void* task){
        load = (struct load_data*) task;

        assert(load != NULL);
        seg = (struct seg_data *)calloc(1, sizeof(struct seg_data));

        seg->name = load->name;

        seg->width = load->width;
        seg->height = load->height;
        seg->HSV = load->HSV;
        image_segment(&seg->mask, &seg->nrgn, load->RGB, load->width, load->height);

        free(load->RGB);
        free(load);

        return (void*) seg;
    }

};

class Extract: public ff::ff_node{
private:
    struct seg_data *seg;
    struct extract_data *extract;
public:
    void* svc(void* task){
        seg = (struct seg_data*) task;

        assert(seg != NULL);
        extract = (struct extract_data *)calloc(1, sizeof(struct extract_data));

        extract->name = seg->name;

        image_extract_helper(seg->HSV, seg->mask, seg->width, seg->height, seg->nrgn, &extract->ds);

        free(seg->mask);
        free(seg->HSV);
        free(seg);

        return (void*) extract;
    }
};

class Vec: public ff::ff_node{
private:
    struct extract_data *extract;
    struct vec_query_data *vec;
    cass_query_t query;
public:
    void* svc(void* task){
        extract = (struct extract_data*) task;

        assert(extract != NULL);
        vec = (struct vec_query_data *)calloc(1, sizeof(struct vec_query_data));
        vec->name = extract->name;

        memset(&query, 0, sizeof query);
        query.flags = CASS_RESULT_LISTS | CASS_RESULT_USERMEM;

        vec->ds = query.dataset = &extract->ds;
        query.vecset_id = 0;

        query.vec_dist_id = vec_dist_id;

        query.vecset_dist_id = vecset_dist_id;

        query.topk = 2*top_K;

        query.extra_params = extra_params;

        cass_result_alloc_list(&vec->result, vec->ds->vecset[0].num_regions, query.topk);

        //	cass_result_alloc_list(&result_m, 0, T1);
        //	cass_table_query(table, &query, &vec->result);
        cass_table_query(table, &query, &vec->result);

        return (void*) vec;
    }
};

class Rank: public ff::ff_node{
private:
    struct vec_query_data *vec;
    struct rank_data *rank;
    cass_result_t *candidate;
    cass_query_t query;
public:
    void* svc(void* task){
        vec = (struct vec_query_data*) task;
        assert(vec != NULL);

        rank = (struct rank_data *)calloc(1, sizeof(struct rank_data));
        rank->name = vec->name;

        query.flags = CASS_RESULT_LIST | CASS_RESULT_USERMEM | CASS_RESULT_SORT;
        query.dataset = vec->ds;
        query.vecset_id = 0;

        query.vec_dist_id = vec_dist_id;

        query.vecset_dist_id = vecset_dist_id;

        query.topk = top_K;

        query.extra_params = NULL;

        candidate = cass_result_merge_lists(&vec->result, (cass_dataset_t *)query_table->__private, 0);
        query.candidate = candidate;


        cass_result_alloc_list(&rank->result, 0, top_K);
        cass_table_query(query_table, &query, &rank->result);

        cass_result_free(&vec->result);
        cass_result_free(candidate);
        free(candidate);
        cass_dataset_release(vec->ds);
        free(vec->ds);
        free(vec);
        return (void*) rank;
    }
};

class Out: public ff::ff_node{
private:
    struct rank_data *rank;
public:
    void* svc(void* task){
        rank = (struct rank_data*) task;

        assert(rank != NULL);

        fprintf(fout, "%s", rank->name);

        ARRAY_BEGIN_FOREACH(rank->result.u.list, cass_list_entry_t p)
        {
            char *obj = NULL;
            if (p.dist == HUGE) continue;
            cass_map_id_to_dataobj(query_table->map, p.id, &obj);
            assert(obj != NULL);
            fprintf(fout, "\t%s:%g", obj, p.dist);
        } ARRAY_END_FOREACH;

        fprintf(fout, "\n");

        cass_result_free(&rank->result);
        free(rank->name);
        free(rank);

        cnt_dequeue++;

        //fprintf(stderr, "(%d,%d)\n", cnt_enqueue, cnt_dequeue);
        return GO_ON;
    }
};

int main (int argc, char *argv[])
{
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

    if (argc < 8)
    {
        printf("%s <database> <table> <query dir> <top K> <depth> <n> <out>\n", argv[0]);
        return 0;
    }

    db_dir = argv[1];
    table_name = argv[2];
    query_dir = argv[3];
    top_K = atoi(argv[4]);

    DEPTH = atoi(argv[5]);
    nthreads = atoi(argv[6]);

    output_path = argv[7];

    fout = fopen(output_path, "w");
    assert(fout != NULL);

    cass_init();

    ret = cass_env_open(&env, db_dir, 0);
    if (ret != 0) { printf("ERROR: %s\n", cass_strerror(ret)); return 0; }

    vec_dist_id = cass_reg_lookup(&env->vec_dist, "L2_float");
    assert(vec_dist_id >= 0);

    vecset_dist_id = cass_reg_lookup(&env->vecset_dist, "emd");
    assert(vecset_dist_id >= 0);

    i = cass_reg_lookup(&env->table, table_name);


    table = query_table = cass_reg_get(&env->table, i);

    i = table->parent_id;

    if (i >= 0)
    {
        query_table = cass_reg_get(&env->table, i);
    }

    if (query_table != table) cass_table_load(query_table);

    cass_map_load(query_table->map);

    cass_table_load(table);

    image_init(argv[0]);

    stimer_tick(&tmr);

    cnt_enqueue = cnt_dequeue = 0;
    printf("Threads,Time,JoulesCpuAll,JoulesCoresAll,JoulesDramAll,JoulesGraphicAll,PCM_energy,NVML_Energy\n");
    for (int thread = 2; thread <= nthreads; thread+=2) {
        printf("%d,", thread);
#ifdef ENABLE_PARSEC_HOOKS
        __parsec_roi_begin();
#endif
        ff::ff_pipeline p;
        ff::ff_farm<> segFarm, extFarm, vecFarm, rankFarm;
        std::vector < ff::ff_node * > segW, extW, vecW, rankW;
        segFarm.cleanup_all();
        extFarm.cleanup_all();
        vecFarm.cleanup_all();
        rankFarm.cleanup_all();
        for (size_t i = 0; i < thread; i++) {
            segW.push_back(new Seg());
            extW.push_back(new Extract());
            vecW.push_back(new Vec());
            rankW.push_back(new Rank());
        }
        segFarm.add_emitter(new Load());
        segFarm.add_workers(segW);
        extFarm.add_workers(extW);
        vecFarm.add_workers(vecW);
        rankFarm.add_workers(rankW);
        segFarm.remove_collector();
        extFarm.setMultiInput();
        extFarm.remove_collector();
        vecFarm.setMultiInput();
        vecFarm.remove_collector();
        rankFarm.setMultiInput();
        rankFarm.add_collector(new Out());

#ifdef ENABLE_FF_ONDEMAND
        segFarm.set_scheduling_ondemand();
        extFarm.set_scheduling_ondemand();
        vecFarm.set_scheduling_ondemand();
        rankFarm.set_scheduling_ondemand();
#endif
        p.add_stage(&segFarm);
        p.add_stage(&extFarm);
        p.add_stage(&vecFarm);
        p.add_stage(&rankFarm);

        p.run_and_wait_end();

        assert(cnt_enqueue == cnt_dequeue);
#ifdef ENABLE_PARSEC_HOOKS
        __parsec_roi_end();
#endif
    }

    stimer_tuck(&tmr, "QUERY TIME");

    ret = cass_env_close(env, 0);
    if (ret != 0) { printf("ERROR: %s\n", cass_strerror(ret)); return 0; }

    cass_cleanup();

    image_cleanup();

    fclose(fout);

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_end();
#endif
    return 0;
}

