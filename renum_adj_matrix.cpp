#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <atomic>

// Read from a file in the form of a adjacency matrix into a binary string.
void read_from_file(const std::string&, std::string*);

// Creates a file ReMatrix_N (N is the index) and fills it with
// the adjacency matrix which corresponds to the given binary string.
void write_as_matrix_to_file(std::string);

// Pushes current string to queue, and generates next. Exits when generates all permutations.
void produce_by_lexicographical_order(const std::string&, unsigned int);

// Creates threads to use the elements in queue for processing. Exits when all consumer threads are finished.
void consume_by_the_same_order();

// The number of rows in the original matrix. Used in
// @write_as_matrix_to_file for correctly processing the input.
int rows = 0;

// A flag to tell consumer that producer still works even if the queue is empty.
std::atomic<bool> done = false;

// Maximum consumer threads to use.
const unsigned int max_thread_count = std::thread::hardware_concurrency();
unsigned int thread_count = max_thread_count;
std::condition_variable thread_cv;
std::mutex thread_mutex;

// Index of the generated matrix
int matrix_index = 0;

// Queue in which the binary strings are collected ( products for consumers ).
std::queue<std::string>* perms;
std::mutex perms_mutex;
std::condition_variable perms_cv;

// Output directory
std::string out_dir;

int main(int argc, char* argv[])
{
	// Require 3 arguments [input file, output directory, memory limit]
	if (argc < 3)
		return -1;

	system(("mkdir " + static_cast<std::string>(argv[2])).c_str());
	out_dir = argv[2];

	std::string* bin_string = new std::string();
	read_from_file(argv[1], bin_string);
	unsigned int max_count = std::atoi(argv[3]) / (bin_string->length() * 2) * 1000000;

	perms = new std::queue<std::string>();
	std::thread p(produce_by_lexicographical_order, std::ref(*bin_string), max_count);
	std::thread c(consume_by_the_same_order);
	c.join();
	p.join();

	delete perms;
	delete bin_string;
	std::cout << matrix_index << std::endl;
	return 0;
}

void produce_by_lexicographical_order(const std::string& status_quo, unsigned int max_count)
{
	std::string quo = std::string(status_quo);
	do {
		std::unique_lock<std::mutex> ul(perms_mutex);
		perms_cv.wait_for(ul, std::chrono::seconds(1),[&max_count]{ return perms->size() < max_count;});
		perms->push(quo);
		ul.unlock();

		std::next_permutation(quo.begin(),quo.end());
	} while (status_quo != quo);

	done = true;
}

void consume_by_the_same_order() {
	perms_mutex.lock();
	while (!done || !perms->empty())
	{
		perms_mutex.unlock();

		std::unique_lock<std::mutex> ul(thread_mutex);
		thread_cv.wait(ul, [] { return thread_count > 0; });
		--thread_count;
		ul.unlock();

		std::unique_lock<std::mutex> pl(perms_mutex);
		perms_cv.wait_for(pl, std::chrono::seconds(1), []{ return !perms->empty();});
		std::thread temp(write_as_matrix_to_file, perms->front());
		perms->pop();
		pl.unlock();
		temp.detach();

		perms_mutex.lock();
	}
	perms_mutex.unlock();

	std::unique_lock<std::mutex> ul(thread_mutex);
	thread_cv.wait(ul, [] { return thread_count == max_thread_count;});
	ul.unlock();
}

void read_from_file(const std::string& path, std::string* bin_string)
{
	std::fstream file(path, std::fstream::in);
	char buf = 0;
	int m_lim = 0;
	int lim = 0;
	while (file >> std::noskipws >> buf)
	{
		if (lim > 0 && (buf == '0' || buf == '1'))
		{
			*bin_string += buf;
			--lim;
		}
		else if (lim <= 0)
		{
			lim = ++m_lim;
			++rows;
			file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
		}
	}
	file.close();
}

void write_as_matrix_to_file(std::string dump)
{
	std::ofstream file;
	file.open("/matrix_" + std::to_string(++matrix_index));

	int i = 0;
	int j = 0;
	while (j < rows)
	{
		i = 0;
		while ( i < rows )
		{
			if (i == j)
				file << 0;
			else if (i > j)
				file << dump[i*(i-1)/2 + j];
			else
				file << dump[j*(j-1)/2 + i];
			file << " ";
			i++;
		}
		file << std::endl;
		++j;
	}
	file.close();

	thread_mutex.lock();
	thread_count++;
	thread_mutex.unlock();
	thread_cv.notify_one();
}

