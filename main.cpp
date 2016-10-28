#include <iostream>
#include <sys/mman.h>
#include <omp.h>
#include "graph.h"
#include "openmp_sssp.hpp"

int main(int args, char **argv)
{
	typedef long dist_t;
	typedef long index_t;
	typedef long vertex_t;
	typedef char update_t;

	std::cout<<"Input: ./exe beg csr weight thread_count\n";
	if(args!=5){std::cout<<"Wrong input\n"; return -1;}
	
	const char *beg_file=argv[1];
	const char *csr_file=argv[2];
	const char *weight_file=argv[3];
	const int thread_count=atoi(argv[4]);


	int ft_count, ft_count_next;
	vertex_t src=0;

	graph<long, long, int, vertex_t,index_t, dist_t>
	*ginst = new graph
	<long, long, int, vertex_t, index_t, dist_t>
	(beg_file,csr_file,weight_file);
	
	index_t vert_count=ginst->vert_count;
	index_t edge_count=ginst->edge_count;
	index_t *beg_pos=ginst->beg_pos;
	vertex_t *csr=ginst->csr;
	dist_t *weight=ginst->weight;

	openmp_sssp
		<dist_t, index_t, vertex_t, update_t>
		(src,
		 beg_pos,
		 csr,
		 weight,
		 vert_count,
		 edge_count,
		 thread_count
		 );
}

