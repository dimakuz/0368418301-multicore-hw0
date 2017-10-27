//    Licensed under the Apache License, Version 2.0 (the "License"); you may
//    not use this file except in compliance with the License. You may obtain
//    a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
//    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
//    License for the specific language governing permissions and limitations
//    under the License.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

#define SPLIT_THRESH 1024

static size_t partition(uint64_t *input, size_t len) {
	uint64_t pivot = input[0];
	size_t pivot_idx = 0;

	for (size_t i = 1; i < len; i++) {
		if (input[i] < pivot)
			pivot_idx++;
	}

	swap(input[0], input[pivot_idx]);

	size_t next_ge = pivot_idx + 1;
	for (size_t i = 0; i < pivot_idx; i++) {
		if (input[i] < pivot)
			continue;

		while (next_ge < len && input[next_ge] >= pivot)
			next_ge++;

		swap(input[i], input[next_ge]);
	}

	return pivot_idx;
}

class work {
public:
	work(uint64_t *vec, size_t len) : vec(vec), len(len) {}

	uint64_t *vec;
	size_t len;
};

class work_queue {
public:
	inline work *steal() {
		unique_lock<mutex> ul(this->mtx);

		if (this->queue.empty())
			return NULL;

		work *w = this->queue.back();
		this->queue.pop_back();

		return w;
	}

	inline work *take() {
		unique_lock<mutex> ul(this->mtx);

		if (this->queue.empty())
			return NULL;

		work *w = this->queue.front();
		this->queue.pop_front();

		return w;
	}

	inline void put(work *w) {
		unique_lock<mutex> ul(this->mtx);
		this->queue.push_front(w);
	}

private:
	deque<work *> queue;
	mutex mtx;
};

struct ctx {
public:
	inline ctx(unsigned int nprocs, size_t left)
		: nprocs(nprocs), left(left)
	{
		for (unsigned int i = 0; i < this->nprocs; i++) {
			this->work_queues.push_back(new work_queue());
		}
	}

	virtual ~ctx() {
		for (unsigned int i = 0; i < this->nprocs; i++) {
			delete this->work_queues[i];
		}
	}
	unsigned int nprocs;
	vector<work_queue *> work_queues;
	atomic_uint left;
};

void work_thread(ctx &ctx, unsigned int id) {
	while (ctx.left) {
		work *cur = ctx.work_queues[id]->take();
		if (!cur) {
			for (size_t i = 0; i < ctx.nprocs; i++) {
				if (i == id)
					continue;

				cur = ctx.work_queues[i]->steal();
				if(cur)
					break;
			}
			if (!cur) {
				this_thread::yield();
				continue;
			}
		}

		size_t len = cur->len;
		uint64_t *input = cur->vec;
		delete cur;

		if (len <= SPLIT_THRESH) {
			sort(input, input + len);
			ctx.left -= len;
			continue;
		}

		size_t mid = partition(input, len);
		ctx.left--;

		if (mid) {
			ctx.work_queues[id]->put(new work(input, mid));
		}
		if (mid != (len - 1)) {
			ctx.work_queues[id]->put(new work(input + mid + 1, len - mid - 1));
		}
	}
}

static void usage(const char *progname) {
	cout << progname << " nprocs input-path" << endl;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		usage(argv[0]);
		return -1;
	}

	size_t nprocs = (size_t) atoi(argv[1]);
	ifstream file(argv[2]);
	uint64_t temp;
	list<uint64_t> lst;

	while (file >> temp) {
		lst.push_back(temp);
	}

	uint64_t *arr = new uint64_t[lst.size()];
	size_t i = 0;
	for (auto itr = lst.begin(); itr != lst.end(); itr++) {
		arr[i++] = *itr;
	}

	auto begin = chrono::high_resolution_clock::now();

	ctx ctx(nprocs, lst.size());
	ctx.work_queues[0]->put(new work(arr, lst.size()));

	vector<thread> threads;
	for (size_t i = 1; i < nprocs; i++) {
		threads.push_back(thread(work_thread, ref(ctx), i));
	}

	work_thread(ctx, 0);

	auto end = chrono::high_resolution_clock::now();

	cout << "QuickSort " << chrono::duration_cast<chrono::microseconds>(end-begin).count() << endl;
	// cerr << chrono::duration_cast<chrono::microseconds>(end-begin).count() << endl;

	for (size_t i = 0; i < lst.size(); i++) {
		cout << arr[i] << endl;
	}
	for (auto itr = threads.begin(); itr != threads.end(); itr++) {
		itr->join();
	}
	return 0;
}
