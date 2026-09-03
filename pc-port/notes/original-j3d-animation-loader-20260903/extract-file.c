#include <nod.h>
#include <stdio.h>
int main(int argc, char** argv) {
 if (argc != 4) return 2;
 NodHandle *disc=0, *part=0, *file=0;
 if (nod_disc_open(argv[1], 0, &disc) || nod_disc_open_partition_kind(disc,NOD_PARTITION_KIND_DATA,0,&part)) return 3;
 enum NodNodeKind kind; uint32_t size;
 uint32_t index=nod_partition_find_file(part,argv[2],&kind,&size);
 if (index==NOD_FST_STOP || nod_partition_open_file(part,index,&file)) return 4;
 FILE* out=fopen(argv[3],"wb"); if(!out) return 5;
 unsigned char bytes[65536]; int64_t got;
 while((got=nod_read(file,bytes,sizeof bytes))>0) if(fwrite(bytes,1,got,out)!=(size_t)got) return 6;
 fclose(out); nod_free(file); nod_free(part); nod_free(disc);
 return got<0 ? 7 : 0;
}
