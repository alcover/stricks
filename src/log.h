#ifndef LOG_H
#define LOG_H

	#ifdef ENABLE_LOG
		#define LOG(...)	do{ printf(__VA_ARGS__); printf("\n"); 	fflush(stdout);} while(0)
		#define LOGI(i) 	do{ printf("%d\n", (int)(i)); 			fflush(stdout);} while(0)
		#define LOGVS(id)   do{ printf("%s : '%s'\n", #id, id);     fflush(stdout);} while(0)
		#define LOGVI(id)   do{ printf("%s : %d\n", #id, (int)id);  fflush(stdout);} while(0)
	#else
		#define LOG(...)	((void)0)
		#define LOGI(id) 	((void)0)
		#define LOGVS(id) 	((void)0)
		#define LOGVI(id) 	((void)0)
	#endif

	#define ERR(args...) fprintf(stderr,args)

#endif