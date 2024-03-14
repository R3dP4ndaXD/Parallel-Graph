// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
int count;

/* TODO: Define graph task argument. */
struct task_args {
	unsigned int idx;
};

static void process_node(void *arg)
{
	/* TODO: Implement thread-pool based processing of graph. */
	struct task_args *args;
	os_node_t *node;

	if (arg == (void *) 0) {
		node = graph->nodes[0];
	} else {
		args = (struct task_args *)arg;
		node = graph->nodes[args->idx];
	}
	pthread_mutex_lock(&mutex1);
	if (graph->visited[node->id] == NOT_VISITED) {
		graph->visited[node->id] = PROCESSING;
		count++;
		sum += node->info;
		pthread_mutex_unlock(&mutex1);
		pthread_mutex_lock(&mutex2);
		for (unsigned int i = 0; i < node->num_neighbours; i++) {
			if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
				struct task_args *new_args = malloc(sizeof(struct task_args));

				new_args->idx = node->neighbours[i];
				os_task_t *t = create_task(process_node, (void *)new_args, free);

				enqueue_task(tp, t);
			}
		}
		pthread_mutex_unlock(&mutex2);
		pthread_mutex_lock(&mutex1);
		graph->visited[node->id] = DONE;
		count--;
		if (!count) {
			tp->done = 1;
			pthread_cond_signal(&tp->working);
		}
		pthread_mutex_unlock(&mutex1);
	} else {
		pthread_mutex_unlock(&mutex1);
	}
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	pthread_mutex_init(&mutex1, NULL);
	pthread_mutex_init(&mutex2, NULL);

	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);

	return 0;
}
