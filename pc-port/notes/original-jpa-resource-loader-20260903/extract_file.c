#include <nod.h>
#include <stdio.h>
#include <string.h>
static uint32_t list(uint32_t i, enum NodNodeKind k,const char*n,uint32_t s,void*u) { if(strstr(n,"ffect")||strstr(n,"article"))fprintf(stderr,"%u %u %s (%u)\n",i,k,n,s);return i+1; }
static int check(enum NodResult r) { if (r == NOD_RESULT_OK) return 1; fprintf(stderr,"nod: %s\n",nod_error_message()); return 0; }
int main(int argc,char **argv) {
 if(argc!=4) return 2;
 NodHandle *disc=0,*part=0,*file=0;
 if(!check(nod_disc_open(argv[1],0,&disc))||!check(nod_disc_open_partition_kind(disc,NOD_PARTITION_KIND_DATA,0,&part))) return 1;
 nod_partition_iterate_fst(part,list,0);
 enum NodNodeKind kind; uint32_t length=0, index=nod_partition_find_file(part,argv[2],&kind,&length);
 if(index==NOD_FST_STOP||!check(nod_partition_open_file(part,index,&file))) return 1;
 FILE *out=fopen(argv[3],"wb"); if(!out)return 1;
 uint8_t buffer[65536]; size_t total=0; int64_t count;
 while((count=nod_read(file,buffer,sizeof(buffer)))>0) {if(fwrite(buffer,1,(size_t)count,out)!=(size_t)count)return 1;total+=(size_t)count;}
 int failed=count<0||fclose(out)||total!=length;
 fprintf(stderr,"%s: %zu/%u bytes\n",argv[2],total,length);
 nod_free(file);nod_free(part);nod_free(disc);return failed;
}
