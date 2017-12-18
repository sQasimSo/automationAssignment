#ifndef GRID_API_DOT_H__
#define GRID_API_DOT_H__

#include "ic_api.h"

#ifdef __cplusplus
extern "C" {

#endif

#ifdef _WINDLL
#define IC_API __declspec(dllexport)
#else
#define IC_API extern
#endif


	/* VUGen grid client: */

	typedef struct grid_client_t {
		ic_result latest_result;                                  /* Keeps status of the latest executed command */
		void(*exception_callback)(struct grid_client_t *); /* Callback to be invoked on a failure during a command execution */
		void      *handler;                                       /* For internal use */
	} grid_client;

	static void grid_process_exception(grid_client *gridClient) {
		void(*orig_callback)(grid_client *);
		if (gridClient != NULL && gridClient->latest_result.status != IC_DONE && (orig_callback = gridClient->exception_callback) != 0) {
			gridClient->exception_callback = 0;
			(*orig_callback)(gridClient);
			gridClient->exception_callback = orig_callback;
		}
	}

	static const char *grid_init(grid_client *gridClient, /* A pointer to an 'grid_client' handler (may not be NULL!) */
		const char * userName,
		const char * password,
		const char * projectName,
		const char * domain,
		int port,
		ic_bool isSecured,
		void(*exception_callback)(struct grid_client_t *)) {

		IC_API const char *grid_impl_init(grid_client *, ic_log *, const char *, const char *, const char *, const char *, int, ic_bool, void(*)(struct grid_client_t *));
		ic_log log = { 0 };
		const char *retval = gridClient == NULL ? "The first argument must be non-NULL" : 
											      grid_impl_init(gridClient, &log, userName, password, projectName, domain, port, isSecured, exception_callback);
		if (retval != NULL)
			lr_error_message("[ERROR]: grid_init() ended with the following report: %s", retval);
		ic_output_log(&log);
		if (gridClient != NULL && gridClient->latest_result.status != IC_DONE && exception_callback != 0) {
			gridClient->exception_callback = 0;
			(*exception_callback)(gridClient);
			gridClient->exception_callback = exception_callback;
		}
		return retval;
	}


	static const char *grid_lock_device_for_execution(grid_client *gridClient, /* A pointer to an 'image_client' handler (may not be NULL!) */
		image_client *client,
		const char * testName,
		const char * deviceQuery,
		int reservationTimeInMinutes,
		long long timeout,
		void(*exception_callback)(struct image_client_t *)) {

		IC_API const char *grid_lock_device_for_execution_impl(grid_client *, image_client *, ic_log *, const char *, const char *, int, long long, void(*)(struct image_client_t *));
		ic_log log = { 0 };
		const char *retval = gridClient == NULL ? "The first argument must be non-NULL" : 
			grid_lock_device_for_execution_impl(gridClient, client, &log, testName, deviceQuery, reservationTimeInMinutes, timeout, exception_callback);
		if (retval != NULL)
			lr_error_message("[ERROR]: grid_lock_device_for_execution() ended with the following report: %s", retval);
		ic_output_log(&log);
		grid_process_exception(gridClient);

		return retval;
	}



#ifdef __cplusplus
}
#endif

#endif /* GRID_API_DOT_H__ */
