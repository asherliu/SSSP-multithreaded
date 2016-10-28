#include <iostream>
#include <sys/mman.h>
#include <omp.h>
#include "graph.h"
#define INFTY	((int)1<<30)

template<typename dist_t,
	typename index_t,
	typename vertex_t,
	typename update_t>
void openmp_sssp(
		vertex_t src,
		index_t *beg_pos,
		vertex_t *csr,
		dist_t *weight,
		index_t vert_count,
		index_t edge_count,
		index_t thread_count
){
	dist_t *dist;
	update_t *update_map;

	dist=(dist_t *)mmap(NULL,sizeof(dist_t)*vert_count,
			PROT_READ | PROT_WRITE,
			//MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_1GB,
			//MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB ,
			MAP_PRIVATE | MAP_ANONYMOUS ,
			0,0);
	perror("dist: mmap");

	update_map=(update_t*)mmap(NULL,sizeof(update_t)*vert_count,
			PROT_READ | PROT_WRITE,
			//MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_1GB,
			//MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB ,
			MAP_PRIVATE | MAP_ANONYMOUS ,
			0,0);
	perror("update_map: mmap");
	
	for(int i=0;i<vert_count;++i)
	{
		dist[i]=INFTY;
		update_map[i]=-1;
	}
	
	dist[src]=0;
	update_map[src]=0;
	index_t *front_comm=new index_t[thread_count];
	index_t *explore_comm=new index_t[thread_count];
	index_t *work_comm=new index_t[thread_count];

#pragma omp parallel \
	num_threads(thread_count)
	{
		update_t level=0;
		bool flag=false;
		index_t tid=omp_get_thread_num();
		index_t step=vert_count/thread_count;
		index_t vert_beg=tid*step;
		index_t vert_end=vert_beg+step;
		if(tid==thread_count-1) vert_end=vert_count;
		
		double stm=wtime();
		while(true)
		{
			index_t front_count=0;
			index_t explore_count=0;
			index_t work_count=0;

			double tm=wtime();

			for(index_t vert_id=vert_beg;vert_id<vert_end;vert_id++)
			{
				if(update_map[vert_id]==level)//updated last level
				{
					explore_count++;		
					dist_t my_dist=dist[vert_id];
					index_t my_beg=beg_pos[vert_id];
					index_t my_end=beg_pos[vert_id+1];
					work_count+=my_end-my_beg;

#pragma unroll
					for(;my_beg<my_end;my_beg++)
					{
						vertex_t nebr=csr[my_beg];
						dist_t edge_weight=weight[my_beg];

						do{
							dist_t nebr_dist=dist[nebr];
							if(my_dist+edge_weight>=nebr_dist) 
								break;

							//atomic
							flag=__sync_bool_compare_and_swap(dist+nebr,
									nebr_dist,my_dist+edge_weight);

							if(flag == true)
							{
								update_map[nebr]=level+1;
								front_count++;
							}

						}while(!flag);
					}
				}
			}

			front_comm[tid]=front_count;
			explore_comm[tid]=explore_count;
			work_comm[tid]=work_count;

#pragma omp barrier
			tm=wtime()-tm;
			front_count=0;
			explore_count=0;
			work_count=0;

			for(index_t i=0;i<thread_count;i++)
			{
				front_count+=front_comm[i];
				explore_count+=explore_comm[i];
				work_count+=work_comm[i];
			}


			if(front_count==0) break;
			if(tid==0)
				std::cout<<"Iteration-explorecount-workcount-updatecount-time: "
					<<(int)level<<" "
					<<explore_count<<" "
					<<work_count<<" "
					<<front_count<<" "<<tm<<"\n";
#pragma omp barrier

			level++;
			assert(level<127);

		}

		if(tid==0) std::cout<<"Total time "<<wtime()-stm<<" seconds\n";
	}

	std::cout<<"Check distance: ";
	for(int i=0;i<100;i++)
		std::cout<<dist[i]<<" ";
	std::cout<<"\n";
	return ;	
}
