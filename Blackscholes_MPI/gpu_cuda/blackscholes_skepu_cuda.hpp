/* This File contains the functions and program logic to calculate 'Option Price' for the two types
* of options called 'put' and 'Call' of European call options on a stock.
*
*
* Code Copyright by : PARSEC BENCHMARK
* MPI Implementation by : Jahanzeb Maqbool | NUST School of Electrical Engineering and Computer Science.
* SkePU integration by : Sami Djouadi | University of St Andrews.
*
*/

#include <stdio.h>
#include <math.h>
#include "OptionDataStruct.h"
#include <skepu>



/* ////////////////////////////////////////////////////////////////////////////// */
/*   Polynomial approximation of cumulative normal distribution function  (CNDF)  */
/* ////////////////////////////////////////////////////////////////////////////// */

#define inv_sqrt_2xPI 0.39894228040143270286

float CNDF(float InputX);

///////////////////////////////////////////////////////////////////////////////
// Black-Scholes formula for both call and put
///////////////////////////////////////////////////////////////////////////////

float computeFormula(float sptprice,
                     float strike, float rate, float volatility,
                     float time, int otype);

float mapFunction(OptionData data_e);

/* Program Entry point for calculation of 'option prices' */
void calculateOptionPrice(OptionData *data, float *optionPrices, int chunkOptions,
                          int numIteration);


