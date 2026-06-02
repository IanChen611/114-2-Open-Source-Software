#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 9
#define TOTAL_THREADS 27

typedef struct {
	int type;
	int index;
	const int (*board)[SIZE];
	int *results;
	pthread_mutex_t *print_mutex;
} ThreadArgs;

static void print_board(const int (*board)[SIZE]) {
	for (int row = 0; row < SIZE; ++row) {
		for (int col = 0; col < SIZE; ++col) {
			printf("%d%c", board[row][col], col == SIZE - 1 ? '\n' : ' ');
		}
	}
}

static int read_board(int board[SIZE][SIZE], const char *path) {
	FILE *fp = path ? fopen(path, "r") : stdin;
	if (!fp) {
		fprintf(stderr, "Cannot open file: %s\n", path);
		return 0;
	}

	for (int row = 0; row < SIZE; ++row) {
		for (int col = 0; col < SIZE; ++col) {
			if (fscanf(fp, "%d", &board[row][col]) != 1) {
				fprintf(stderr, "Invalid board format at row %d col %d\n", row, col);
				if (path) fclose(fp);
				return 0;
			}
		}
	}

	if (path) fclose(fp);
	return 1;
}

static int validate_row(const int (*board)[SIZE], int row) {
	int seen[SIZE + 1] = {0};

	for (int column = 0; column < SIZE; ++column) {
		int value = board[row][column];

		if (value < 1 || value > SIZE) {
			return 0;
		}
		if (seen[value]) {
			return 0;
		}

		seen[value] = 1;
	}

	return 1;
}

static int validate_column(const int (*board)[SIZE], int column) {
	int seen[SIZE + 1] = {0};

	for (int row = 0; row < SIZE; ++row) {
		int value = board[row][column];

		if (value < 1 || value > SIZE) {
			return 0;
		}
		if (seen[value]) {
			return 0;
		}

		seen[value] = 1;
	}

	return 1;
}

static int validate_subgrid(const int (*board)[SIZE], int subgrid) {
	int seen[SIZE + 1] = {0};
	int start_row = (subgrid / 3) * 3;
	int start_column = (subgrid % 3) * 3;

	for (int row = start_row; row < start_row + 3; ++row) {
		for (int column = start_column; column < start_column + 3; ++column) {
			int value = board[row][column];

			if (value < 1 || value > SIZE) {
				return 0;
			}
			if (seen[value]) {
				return 0;
			}

			seen[value] = 1;
		}
	}

	return 1;
}

static void *worker(void *arg) {
	ThreadArgs *thread_args = (ThreadArgs *)arg;
	int result = -1;

	if (thread_args->type == 0) {
		result = validate_row((const int (*)[SIZE])thread_args->board, thread_args->index);
	} else if (thread_args->type == 1) {
		result = validate_column((const int (*)[SIZE])thread_args->board, thread_args->index);
	} else {
		result = validate_subgrid((const int (*)[SIZE])thread_args->board, thread_args->index);
	}

	thread_args->results[thread_args->type * SIZE + thread_args->index] = result;

	return NULL;
}

int main(int argc, char *argv[]) {
	int board[SIZE][SIZE];

	if (argc >= 2) {
		if (!read_board(board, argv[1])) {
			return EXIT_FAILURE;
		}
	} else {
		/* fallback: use a built-in valid board for quick testing */
		int default_board[SIZE][SIZE] = {
			{5, 3, 4, 6, 7, 8, 9, 1, 2},
			{6, 7, 2, 1, 9, 5, 3, 4, 8},
			{1, 9, 8, 3, 4, 2, 5, 6, 7},
			{8, 5, 9, 7, 6, 1, 4, 2, 3},
			{4, 2, 6, 8, 5, 3, 7, 9, 1},
			{7, 1, 3, 9, 2, 4, 8, 5, 6},
			{9, 6, 1, 5, 3, 7, 2, 8, 4},
			{2, 8, 7, 4, 1, 9, 6, 3, 5},
			{3, 4, 5, 2, 8, 6, 1, 7, 9}
		};
		memcpy(board, default_board, sizeof(board));
	}

	pthread_t threads[TOTAL_THREADS];
	ThreadArgs args[TOTAL_THREADS];
	int results[TOTAL_THREADS] = {0};
	pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
	int threads_created = 0;

	printf("Sudoku board:\n");
	print_board((const int (*)[SIZE])board);
	printf("\n");

	for (int i = 0; i < TOTAL_THREADS; ++i) {
		args[i].type = i / SIZE;
		args[i].index = i % SIZE;
		args[i].board = (const int (*)[SIZE])board;
		args[i].results = results;
		args[i].print_mutex = &print_mutex;

		if (pthread_create(&threads[i], NULL, worker, &args[i]) != 0) {
			fprintf(stderr, "Failed to create thread %d\n", i);
			for (int j = 0; j < threads_created; ++j) {
				pthread_join(threads[j], NULL);
			}
			pthread_mutex_destroy(&print_mutex);
			return EXIT_FAILURE;
		}
		threads_created++;
	}

	for (int i = 0; i < TOTAL_THREADS; ++i) {
		if (pthread_join(threads[i], NULL) != 0) {
			fprintf(stderr, "Failed to join thread %d\n", i);
			pthread_mutex_destroy(&print_mutex);
			return EXIT_FAILURE;
		}

		const char *type_name =
			args[i].type == 0 ? "row" :
			args[i].type == 1 ? "column" : "subgrid";
		printf("Thread finished: %s %d -> %s\n",
		       type_name, args[i].index + 1,
		       results[i] == 1 ? "valid" : "invalid");
	}

	pthread_mutex_destroy(&print_mutex);

	bool all_valid = true;
	bool has_pending = false;

	for (int i = 0; i < TOTAL_THREADS; ++i) {
		if (results[i] == 0) {
			all_valid = false;
		}
		if (results[i] < 0) {
			has_pending = true;
		}
	}

	printf("\n");
	if (has_pending) {
		printf("Some validation logic is still pending.\n");
	} else if (all_valid) {
		printf("Sudoku solution is valid.\n");
	} else {
		printf("Sudoku solution is invalid.\n");
	}

	return EXIT_SUCCESS;
}
