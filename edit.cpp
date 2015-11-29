trans_t global_id;
map<trans_t,transaction_info> transactions_map;
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases)
{
	transaction_info tid_info;
	tid_info.rvm = rvm;
	map<string,segment_t>::iterator seg_it;
	int i, flag =0;

	/*
	 * Checking if segments reqd already in transactions, or unmapped
	 */
	for (i=0;i<numsegs;i++){
		flag = 0;
		for(seg_it=rvm->segment_map.begin(); seg_it!=rvm->segment_map.end(); seg_it++){
			if (seg_it->second.address == segbases[i]){
				flag =1;
				if (!seg_it->second.is_valid){
					printf("Segment is not mapped. Cannot start transaction\n");
					return -1;
				}
				if (seg_it->second.currently_used){
					printf("Segment is being used by another transaction\n");
					return -1;
				}
				//<segbase,segment_t*>
				tid_info.segments[segbases[i]]=&(seg_it->second);
			}
		}
		if (!flag){
			printf("Segment does not exist!\n");
			return -1;
		}
	}

	/*
	 * Marking segments to be modified as currenty used
	 */
	//<segbase,segment_t*>
	map<void*, segment_t*>::iterator it;
	for(it = tid_info.segments.begin(); it != tid_info.segments.end(); it++)
	{
		tid_info.segments[it->first]->currently_used = 1;					
	}

	/*
	 * Adding transaction_info data to global transactions map
	 */
	trans_t trans_id = global_id++;
	transactions_map[trans_id]=tid_info;
	return trans_id;
}