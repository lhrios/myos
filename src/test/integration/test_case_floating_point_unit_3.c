/*
 * Copyright 2022 Luis Henrique O. Rios
 *
 * This file is part of MyOS.
 *
 * MyOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MyOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MyOS. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "test/integration_test.h"

#include "util/math_utils.h"


#define DELTA 0.00001

static double computeMean(double* samples, size_t sampleCount) {
	double sum = 0;
	for (int i = 0; i < sampleCount; i++) {
		sum += samples[i];
	}
	return sum / sampleCount;
}

static double computeStandardDeviation1(double* samples, size_t sampleCount, double mean) {
	double sum = 0;
	for (int i = 0; i < sampleCount; i++) {
		sum += (samples[i] - mean) * (samples[i] - mean);
	}
	return sqrt(sum / sampleCount);
}

static double computeStandardDeviation2(double* samples, size_t sampleCount, double mean) {
	double sum = 0;
	for (int i = 0; i < sampleCount; i++) {
		sum += (samples[i] - mean) * (samples[i] - mean);
	}
	return pow(sum / sampleCount, 0.5);
}

int main(int argc, char** argv) {
	integrationTestConfigureCommonSignalHandlers();

	const int sampleCount = 1024 * 1204 * 6 - 1;
	double* samples = malloc(sizeof(double) * sampleCount);
	assert(samples != NULL);

	for (int i = 0; i < sampleCount; i++) {
		samples[i] = i - (sampleCount / 2);
	}

	double mean = computeMean(samples, sampleCount);
	assert(mathUtilsApproximatelyEquals(mean, 0, DELTA));

	double standardDeviation;
	standardDeviation = computeStandardDeviation1(samples, sampleCount, mean);
	assert(mathUtilsApproximatelyEquals(standardDeviation, 2135438.223778, DELTA));

	standardDeviation = computeStandardDeviation2(samples, sampleCount, mean);
	assert(mathUtilsApproximatelyEquals(standardDeviation, 2135438.223778, DELTA));

	free(samples);

	integrationTestRegisterSuccessfulCompletion(argv[0]);
	return EXIT_SUCCESS;
}
