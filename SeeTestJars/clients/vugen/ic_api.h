#ifndef IC_API_DOT_H__
#define IC_API_DOT_H__

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef _WINDLL
	#define IC_API __declspec(dllexport)
#else
	#define IC_API extern
#endif

#define	IC_MAX_MESSAGE_NUMBER 10000

	/* Complex return types: */

	typedef enum { IC_FALSE, IC_TRUE } ic_bool;

	typedef struct {
		int    length;
		char **array;
	} ic_string_array;


	/* Data types of status of the latest executed command: */

	typedef enum {
		IC_DONE, IC_INTERNAL_EXCEPTION
	} ic_status;

	typedef struct image_client_internal_exception_t {
		const char *message;
		const char *cause_id;
	} ic_internal_exception;

	typedef struct image_client_result_t {
		ic_status status;
		union {
			ic_internal_exception internal_exception;
		} details;
	} ic_result;

	typedef struct image_client_log_t {
		ic_string_array messages;
		const int      *arr_levels;
	} ic_log;

	/* VUGen client: */

	typedef struct image_client_t {
		ic_result latest_result;                                  /* Keeps status of the latest executed command */
		void      (*exception_callback)(struct image_client_t *); /* Callback to be invoked on a failure during a command execution */
		void      *handler;                                       /* For internal use */
	} image_client;


	/* Declarations of auxiliary functions: */
	static void ic_output_log(const ic_log *log);
	static void ic_process_exception(image_client *client);



	/* API of VUGen-client commands: */

	/*
	/// <summary> Initializes an 'image_client' handler. </summary>
	/// <returns> On success: NULL; Otherwise, an error message describing why the initialization has failed </returns>
	/// <documentation>https://docs.experitest.com/display/public/SA/Initializing+A+VUGen+Client</documentation>
	*/
	static const char *ic_init(image_client *client, /* A pointer to an 'image_client' handler (may not be NULL!) */
		                       const char *host, /* The name or the address of the executor host-machine (default="127.0.0.1") */
		                       int port /* The port of the executor (default=8889) */,
							   void (*exception_callback)(struct image_client_t *)) {
		IC_API const char *ic_impl_init(image_client *, ic_log *, const char *, int, ic_bool, void(*)(struct image_client_t *));
		ic_log log = { 0 };
		const char *retval = client == NULL? "The first argument must be non-NULL" : ic_impl_init(client, &log, host, port, IC_TRUE, exception_callback);
		if (retval != NULL)
			lr_error_message("[ERROR]: ic_init() ended with the following report: %s", retval);
		ic_output_log(&log);
		if (client != NULL && client->latest_result.status != IC_DONE && exception_callback != 0) {
			client->exception_callback = 0;
			(*exception_callback)(client);
			client->exception_callback = exception_callback;
		}
		return retval;
	}

	/*
	/// <remark>  Important note: we strongly discourage you from calling this function! 
	///           It exists for compatibility with previous versions of the pruduct.
	/// </remark>
	/// <summary> Initializes an 'image_client' handler without using session IDs. For more information about the session IDs, refer to https://docs.experitest.com/display/public/SA/Working+With+Session+Ids</summary>
	/// <returns> On success: NULL; Otherwise, an error message describing why the initialization has failed </returns>
	/// <documentation>https://docs.experitest.com/display/public/SA/Initializing+A+VUGen+Client</documentation>
	*/
	static const char *ic_init_without_session_ids(image_client *client, /* A pointer to an 'image_client' handler (may not be NULL!) */
		                                           const char *host, /* The name or the address of the executor host-machine (default="127.0.0.1") */
												   int port /* The port of the executor (default=8889) */,
												   void (*exception_callback)(struct image_client_t *)) {
		IC_API const char *ic_impl_init(image_client *, ic_log *, const char *, int, ic_bool, void(*)(struct image_client_t *));
		ic_log log = { 0 };
		const char *retval = client == NULL ? "The first argument must be non-NULL" : ic_impl_init(client, &log, host, port, IC_FALSE, exception_callback);
		if (retval != NULL)
			lr_error_message("[ERROR]: ic_init_without_session_ids() ended with the following report: %s", retval);
		ic_output_log(&log);
		if (client != NULL && client->latest_result.status != IC_DONE && exception_callback != 0) {
			client->exception_callback = 0;
			(*exception_callback)(client);
			client->exception_callback = exception_callback;
		}
		return retval;
	}

	/*
	/// <summary>
	/// Release Controller. In addition, the data allocated for the 'image_client' handler (for example, result strings) will be released.
	/// </summary>
	/// <documentation>https://docs.experitest.com/display/public/SA/ReleaseClient</documentation>
	*/
	static void ic_release_client(image_client *client)
	{
		IC_API void ic_impl_release_client(image_client *, ic_log *);
		IC_API void ic_impl_purge_client_data(image_client *);
		ic_log log = { 0 };
		ic_impl_release_client(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		ic_impl_purge_client_data(client);
	}

	/*
	/// <summary>
	/// Generates a SeeTest report.
	/// </summary>
	/// <returns> Returns the report's folder path </returns>
	/// <documentation>https://docs.experitest.com/display/public/SA/GenerateReport</documentation>
	*/
	static const char *ic_generate_report(image_client *client, /* A pointer to the 'image_client' handler to be released (may not be NULL!) */
		                                  ic_bool release_client /* If IC_FALSE, then device will not be released; If IC_TRUE, it will be and in addition, the data allocated for the 'image_client' handler (for example, result strings) will be released. (default=IC_TRUE) */)
	{
		IC_API const char *ic_impl_generate_report(image_client *, ic_log *, ic_bool);
		IC_API void ic_impl_purge_client_data(image_client *);
		ic_log log = { 0 };
		const char *result = ic_impl_generate_report(client, &log, release_client);
		ic_output_log(&log);
		ic_process_exception(client);
		if (release_client == IC_TRUE)
			ic_impl_purge_client_data(client);
		return result;
	}

	/*
	/// <summary>
	///	             Activates the voice assistant service (e.g. Siri) on the device, and sends the text argument as if it was a spoken directive.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ActivateVoiceAssistance </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_activate_voice_assistance(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                         const char* text /* Text to send (default="") */)
	{
		IC_API void ic_impl_activate_voice_assistance(image_client *client, ic_log *log, const char* text);
		ic_log log = { 0 };
		ic_impl_activate_voice_assistance(client, &log, text);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Add / reserve device. return the name that should be used to access the device.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_add_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                 const char* serial_number /* the device serial number / UDID (default="") */,
	                                 const char* device_name /* the device suggested name (will add an index if the name exists) (default="") */)
	{
		IC_API const char* ic_impl_add_device(image_client *client, ic_log *log, const char* serial_number, const char* device_name);
		ic_log log = { 0 };
		const char* result = ic_impl_add_device(client, &log, serial_number, device_name);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Clear application data
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ApplicationClearData </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_application_clear_data(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* package_name /* The application's package name (default="") */)
	{
		IC_API void ic_impl_application_clear_data(image_client *client, ic_log *log, const char* package_name);
		ic_log log = { 0 };
		ic_impl_application_clear_data(client, &log, package_name);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Close application
	/// </summary>
	/// <returns> successful close </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/ApplicationClose </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_application_close(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                    const char* package_name /* The application's package name (default="") */)
	{
		IC_API ic_bool ic_impl_application_close(image_client *client, ic_log *log, const char* package_name);
		ic_log log = { 0 };
		ic_bool result = ic_impl_application_close(client, &log, package_name);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Capture the current screen and add it to the report.
	/// </summary>
	/// <returns> the path of the captured image file. </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_capture(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_capture(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_capture(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Capture the current screen and add it to the report with the given line.
	/// </summary>
	/// <returns> the path of the captured image file. </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_capture_line(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* line /* the line to be used in the report (default="Capture") */)
	{
		IC_API const char* ic_impl_capture_line(image_client *client, ic_log *log, const char* line);
		ic_log log = { 0 };
		const char* result = ic_impl_capture_line(client, &log, line);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Create new element from image in given device coordinates, with given name and similarity percentage.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_capture_element(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                               const char* name /* New element's name (default="") */,
	                               int x /* Image origin x coordinate */,
	                               int y /* Image origin y coordinate */,
	                               int width /* Image width */,
	                               int height /* Image height */,
	                               int similarity /* the similarity between pictures (default=97) */)
	{
		IC_API void ic_impl_capture_element(image_client *client, ic_log *log, const char* name, int x, int y, int width, int height, int similarity);
		ic_log log = { 0 };
		ic_impl_capture_element(client, &log, name, x, y, width, height, similarity);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Clear device log
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_clear_device_log(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_clear_device_log(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_clear_device_log(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Clears the mock location. Currently supported only on android
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_clear_location(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_clear_location(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_clear_location(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Click an element.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Click </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_click(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                     const char* zone /* Select Zone (default="") */,
	                     const char* element /* Select Element (default="") */,
	                     int index /* Element Order (default=0) */,
	                     int click_count /* Number of Clicks (default=1) */)
	{
		IC_API void ic_impl_click(image_client *client, ic_log *log, const char* zone, const char* element, int index, int click_count);
		ic_log log = { 0 };
		ic_impl_click(client, &log, zone, element, index, click_count);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Click on or near to an element. The offset is specified by x, y.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ClickOffset </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_click_offset(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                            const char* zone /* Select Zone (default="") */,
	                            const char* element /* Select Element (default="") */,
	                            int index /* Element Order (default=0) */,
	                            int click_count /* Number of Clicks (default=1) */,
	                            int x /* Horizontal Offset from Element (default=0) */,
	                            int y /* Vertical Offset from Element (default=0) */)
	{
		IC_API void ic_impl_click_offset(image_client *client, ic_log *log, const char* zone, const char* element, int index, int click_count, int x, int y);
		ic_log log = { 0 };
		ic_impl_click_offset(client, &log, zone, element, index, click_count, x, y);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Click in window X,Y coordinates related to the device screen.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ClickCoordinate </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_click_coordinate(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                int x /* Horizontal coordinate (default=0) */,
	                                int y /* Vertical coordinate (default=0) */,
	                                int click_count /* Number of clicks (default=1) */)
	{
		IC_API void ic_impl_click_coordinate(image_client *client, ic_log *log, int x, int y, int click_count);
		ic_log log = { 0 };
		ic_impl_click_coordinate(client, &log, x, y, click_count);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Search for an element and click on an element near him. The direction can be UP, DOWN, LEFT and RIGHT.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ClickIn </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_click_in(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                        const char* zone /* Select Zone (default="") */,
	                        const char* search_element /* Search Element (default="") */,
	                        int index /* Element index (default=0) */,
	                        const char* direction /* Direction to analyze (default="") */,
	                        const char* click_element_zone /* Click Element Zone (default="") */,
	                        const char* click_element /* Click Element (default="") */,
	                        int click_element_index /* Click Element Index (default=0) */,
	                        int width /* Width of the search (0 indicate until the end/start of the window) (default=0) */,
	                        int height /* Height of the search (0 indicate until the end/start of the window) (default=0) */,
	                        int click_count /* Number of Clicks (default=1) */)
	{
		IC_API void ic_impl_click_in(image_client *client, ic_log *log, const char* zone, const char* search_element, int index, const char* direction, const char* click_element_zone, const char* click_element, int click_element_index, int width, int height, int click_count);
		ic_log log = { 0 };
		ic_impl_click_in(client, &log, zone, search_element, index, direction, click_element_zone, click_element, click_element_index, width, height, click_count);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Clicking on table cell by its header element and row element.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_click_table_cell(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                const char* zone /* Select zone (default="") */,
	                                const char* header_element /* Select table header element (default="") */,
	                                int header_index /* Header element index (default=0) */,
	                                const char* row_element /* Select table row element (default="") */,
	                                int row_index /* Row element index (default=0) */)
	{
		IC_API void ic_impl_click_table_cell(image_client *client, ic_log *log, const char* zone, const char* header_element, int header_index, const char* row_element, int row_index);
		ic_log log = { 0 };
		ic_impl_click_table_cell(client, &log, zone, header_element, header_index, row_element, row_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Close connection to the device.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_close_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_close_device(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_close_device(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Hides the current device screen. If the current device screen is not open, this command does nothing.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_close_device_reflection(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_close_device_reflection(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_close_device_reflection(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Close device keyboard
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_close_keyboard(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_close_keyboard(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_close_keyboard(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <returns> returns the collectSupportData folder path </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_collect_support_data(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           const char* zip_destination /* The path in  which the zip will be saved. If empty, a default path will be used (default="") */,
	                                           const char* application_path /* An absolute path to the application file on the PC (default="") */,
	                                           const char* device /* Device name (default="") */,
	                                           const char* scenario /* Description of your scenario (default="") */,
	                                           const char* expected_result /* The result you expected (default="") */,
	                                           const char* actual_result /* The result you got after running the scenario (default="") */)
	{
		IC_API const char* ic_impl_collect_support_data(image_client *client, ic_log *log, const char* zip_destination, const char* application_path, const char* device, const char* scenario, const char* expected_result, const char* actual_result);
		ic_log log = { 0 };
		const char* result = ic_impl_collect_support_data(client, &log, zip_destination, application_path, device, scenario, expected_result, actual_result);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <returns> returns the collectSupportData folder path </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_collect_support_data2(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                            const char* zip_destination /* The path in  which the zip will be saved. If empty, a default path will be used (default="") */,
	                                            const char* application_path /* An absolute path to the application file on the PC (default="") */,
	                                            const char* device /* Device name (default="") */,
	                                            const char* scenario /* Description of your scenario (default="") */,
	                                            const char* expected_result /* The result you expected (default="") */,
	                                            const char* actual_result /* The result you got after running the scenario (default="") */,
	                                            ic_bool with_cloud_data /* Include cloud data in the support folder (default=IC_TRUE) */,
	                                            ic_bool only_latest_logs /* Include only latest logs in the support folder (default=IC_TRUE) */)
	{
		IC_API const char* ic_impl_collect_support_data2(image_client *client, ic_log *log, const char* zip_destination, const char* application_path, const char* device, const char* scenario, const char* expected_result, const char* actual_result, ic_bool with_cloud_data, ic_bool only_latest_logs);
		ic_log log = { 0 };
		const char* result = ic_impl_collect_support_data2(client, &log, zip_destination, application_path, device, scenario, expected_result, actual_result, with_cloud_data, only_latest_logs);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Execute any of the following device actions:<br/>
	///	 			Home, Back, Power, Landscape, Portrait, Change Orientation, Menu, Unlock, Paste, Volume Up, Volume Down, Recent Apps.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_device_action(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                             const char* action /* Name of action to perform (default="") */)
	{
		IC_API void ic_impl_device_action(image_client *client, ic_log *log, const char* action);
		ic_log log = { 0 };
		ic_impl_device_action(client, &log, action);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Drag an element in a specified zone.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Drag </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_drag(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                    const char* zone /* Select Zone (default="") */,
	                    const char* element /* Select Element to drag (default="") */,
	                    int index /* Element Order (=the number of times the element appears more and above the first time) (default=0) */,
	                    int x_offset /* X drag offset (default=0) */,
	                    int y_offset /* Y drag offset (default=0) */)
	{
		IC_API void ic_impl_drag(image_client *client, ic_log *log, const char* zone, const char* element, int index, int x_offset, int y_offset);
		ic_log log = { 0 };
		ic_impl_drag(client, &log, zone, element, index, x_offset, y_offset);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Drag base on coordinates
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Drag+Coordinates </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_drag_coordinates(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                int x1 /* First point X (default=0) */,
	                                int y1 /* First point Y (default=0) */,
	                                int x2 /* Second point X (default=0) */,
	                                int y2 /* Second point Y (default=0) */,
	                                int time /* Drag time (ms) (default=2000) */)
	{
		IC_API void ic_impl_drag_coordinates(image_client *client, ic_log *log, int x1, int y1, int x2, int y2, int time);
		ic_log log = { 0 };
		ic_impl_drag_coordinates(client, &log, x1, y1, x2, y2, time);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Drop all project information.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_drop(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_drop(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_drop(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Get element property
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementGetProperty </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_element_get_property(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           const char* zone /* Select Zone (default="") */,
	                                           const char* element /* Select Element (default="") */,
	                                           int index /* Element index (default=0) */,
	                                           const char* property /* Property (default="") */)
	{
		IC_API const char* ic_impl_element_get_property(image_client *client, ic_log *log, const char* zone, const char* element, int index, const char* property);
		ic_log log = { 0 };
		const char* result = ic_impl_element_get_property(client, &log, zone, element, index, property);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get table total or visible rows count
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementGetTableRowsCount </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_element_get_table_rows_count(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           const char* zone /* Select Zone (default="") */,
	                                           const char* table_locator /* Select Table Locator (default="") */,
	                                           int table_index /* Table Locator Index (default=0) */,
	                                           ic_bool visible /* Only visible (default=IC_FALSE) */)
	{
		IC_API int ic_impl_element_get_table_rows_count(image_client *client, ic_log *log, const char* zone, const char* table_locator, int table_index, ic_bool visible);
		ic_log log = { 0 };
		int result = ic_impl_element_get_table_rows_count(client, &log, zone, table_locator, table_index, visible);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get text from element
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementGetText </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_element_get_text(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                       const char* zone /* Select Zone (default="") */,
	                                       const char* element /* Select Element (default="") */,
	                                       int index /* Element index (default=0) */)
	{
		IC_API const char* ic_impl_element_get_text(image_client *client, ic_log *log, const char* zone, const char* element, int index);
		ic_log log = { 0 };
		const char* result = ic_impl_element_get_text(client, &log, zone, element, index);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Select an element in a list (first make the element visible)
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementListPick </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_element_list_pick(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                 const char* list_zone /* Select List Zone (default="") */,
	                                 const char* list_locator /* List locator (default="") */,
	                                 const char* element_zone /* Select Element Zone (default="") */,
	                                 const char* element_locator /* Element locator (default="") */,
	                                 int index /* Element index (default=0) */,
	                                 ic_bool click /* If TRUE then click (default=IC_TRUE) */)
	{
		IC_API void ic_impl_element_list_pick(image_client *client, ic_log *log, const char* list_zone, const char* list_locator, const char* element_zone, const char* element_locator, int index, ic_bool click);
		ic_log log = { 0 };
		ic_impl_element_list_pick(client, &log, list_zone, list_locator, element_zone, element_locator, index, click);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Select an element in a list (first make the element visible)
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementListSelect </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_element_list_select(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* list_locator /* List locator (default="") */,
	                                   const char* element_locator /* Element locator (default="") */,
	                                   int index /* Element index (default=0) */,
	                                   ic_bool click /* If TRUE then click (default=IC_TRUE) */)
	{
		IC_API void ic_impl_element_list_select(image_client *client, ic_log *log, const char* list_locator, const char* element_locator, int index, ic_bool click);
		ic_log log = { 0 };
		ic_impl_element_list_select(client, &log, list_locator, element_locator, index, click);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Make the target element visible
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_element_list_visible(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                       const char* list_locator /* List locator (default="") */,
	                                       const char* element_locator /* Element locator (default="") */,
	                                       int index /* Element index (default=0) */)
	{
		IC_API ic_bool ic_impl_element_list_visible(image_client *client, ic_log *log, const char* list_locator, const char* element_locator, int index);
		ic_log log = { 0 };
		ic_bool result = ic_impl_element_list_visible(client, &log, list_locator, element_locator, index);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Scroll table / list to the given row
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementScrollToTableRow </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_element_scroll_to_table_row(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           const char* zone /* Select Zone (default="") */,
	                                           const char* table_locator /* Select Table Locator (default="") */,
	                                           int table_index /* Table Locator Index (default=0) */,
	                                           int row_index /* Row Index (default=0) */)
	{
		IC_API void ic_impl_element_scroll_to_table_row(image_client *client, ic_log *log, const char* zone, const char* table_locator, int table_index, int row_index);
		ic_log log = { 0 };
		ic_impl_element_scroll_to_table_row(client, &log, zone, table_locator, table_index, row_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Send text to an element
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementSendText </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_element_send_text(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                 const char* zone /* Select Zone (default="") */,
	                                 const char* element /* Select Element (default="") */,
	                                 int index /* Element index (default=0) */,
	                                 const char* text /* Text to Send (default="") */)
	{
		IC_API void ic_impl_element_send_text(image_client *client, ic_log *log, const char* zone, const char* element, int index, const char* text);
		ic_log log = { 0 };
		ic_impl_element_send_text(client, &log, zone, element, index, text);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set element property
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementSetProperty </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_element_set_property(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           const char* zone /* Select Zone (default="") */,
	                                           const char* element /* Select Element (default="") */,
	                                           int index /* Element index (default=0) */,
	                                           const char* property /* Property (default="") */,
	                                           const char* value /* The value to set (default="") */)
	{
		IC_API const char* ic_impl_element_set_property(image_client *client, ic_log *log, const char* zone, const char* element, int index, const char* property, const char* value);
		ic_log log = { 0 };
		const char* result = ic_impl_element_set_property(client, &log, zone, element, index, property, value);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Swipe the screen in a given direction
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementSwipe </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_element_swipe(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                             const char* zone /* Select Zone (default="") */,
	                             const char* element /* Select Element (default="") */,
	                             int index /* Element index (default=0) */,
	                             const char* direction /* Direction to swipe (default="") */,
	                             int offset /* Swipe offset (default=0) */,
	                             int time /* Swipe overall time (default=2000) */)
	{
		IC_API void ic_impl_element_swipe(image_client *client, ic_log *log, const char* zone, const char* element, int index, const char* direction, int offset, int time);
		ic_log log = { 0 };
		ic_impl_element_swipe(client, &log, zone, element, index, direction, offset, time);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Swipe a component to search for an element or text.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ElementSwipeWhileNotFound </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_element_swipe_while_not_found(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                                const char* component_zone /* Zone of the container element; (default="") */,
	                                                const char* component_element /* The container element; (default="") */,
	                                                const char* direction /* Direction to swipe; (default="") */,
	                                                int offset /* Swipe offset; (default=0) */,
	                                                int swipe_time /* Swipe operation time; (default=2000) */,
	                                                const char* elementfindzone /* Select Zone of the sought element; (default="") */,
	                                                const char* elementtofind /* Select element to find from the drop-down list OR (for OCR text identification) insert text into the empty box in the drop-down list; (default="") */,
	                                                int elementtofindindex /* The sought element's index (i.e., the number of times the element appears after the first appearance minus two). Index=0 refers to the first appearance of the element; Index=1 refers to the second appearance of the element, etc. (default=0) */,
	                                                int delay /* Time to wait before sending a command (in milliseconds); (default=1000) */,
	                                                int rounds /* Maximum swipe rounds; (default=5) */,
	                                                ic_bool click /* Click the found element if TRUE. (default=IC_TRUE) */)
	{
		IC_API ic_bool ic_impl_element_swipe_while_not_found(image_client *client, ic_log *log, const char* component_zone, const char* component_element, const char* direction, int offset, int swipe_time, const char* elementfindzone, const char* elementtofind, int elementtofindindex, int delay, int rounds, ic_bool click);
		ic_log log = { 0 };
		ic_bool result = ic_impl_element_swipe_while_not_found(client, &log, component_zone, component_element, direction, offset, swipe_time, elementfindzone, elementtofind, elementtofindindex, delay, rounds, click);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			End measuring transaction duration
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/EndTransaction </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_end_transaction(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                               const char* name /* Name of the transaction (default="") */)
	{
		IC_API void ic_impl_end_transaction(image_client *client, ic_log *log, const char* name);
		ic_log log = { 0 };
		ic_impl_end_transaction(client, &log, name);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Exit SeeTest.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_exit(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_exit(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_exit(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Extracts the language files of an application to the specified directory
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ExtractLanguageFiles </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_extract_language_files(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* application /* Language files source application (default="") */,
	                                      const char* directory_path /* Directory's full path where to extract the language files (default="") */,
	                                      ic_bool allow_overwrite /* Whether to allow overwriting existing langauge files in directory (default=IC_TRUE) */)
	{
		IC_API void ic_impl_extract_language_files(image_client *client, ic_log *log, const char* application, const char* directory_path, ic_bool allow_overwrite);
		ic_log log = { 0 };
		ic_impl_extract_language_files(client, &log, application, directory_path, allow_overwrite);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_string_array ic_find_elements(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        const char* zone /* zone of the element (NATIVE / WEB) */,
	                                        const char* parent /* full xpath of parent (empty string / return of previous call) */,
	                                        const char* by /* currently only xpath supported */,
	                                        const char* value /* full xpath of the element to search */)
	{
		IC_API ic_string_array ic_impl_find_elements(image_client *client, ic_log *log, const char* zone, const char* parent, const char* by, const char* value);
		ic_log log = { 0 };
		ic_string_array result = ic_impl_find_elements(client, &log, zone, parent, by, value);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Flick the screen in a given direction
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Flick </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_flick(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                     const char* direction /* Direction to flick (default="") */,
	                     int offset /* Flick offset (default=0) */)
	{
		IC_API void ic_impl_flick(image_client *client, ic_log *log, const char* direction, int offset);
		ic_log log = { 0 };
		ic_impl_flick(client, &log, direction, offset);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Flick from a given point in a given direction
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/FlickCoordinate </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_flick_coordinate(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                int x /* Horizontal coordinate (default=0) */,
	                                int y /* Vertical coordinate (default=0) */,
	                                const char* direction /* Direction to flick (default="") */)
	{
		IC_API void ic_impl_flick_coordinate(image_client *client, ic_log *log, int x, int y, const char* direction);
		ic_log log = { 0 };
		ic_impl_flick_coordinate(client, &log, x, y, direction);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Flick the element in a given direction
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/FlickElement </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_flick_element(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                             const char* zone /* Select Zone (default="") */,
	                             const char* element /* Select Element (default="") */,
	                             int index /* Element Order (default=0) */,
	                             const char* direction /* Direction to flick (default="") */)
	{
		IC_API void ic_impl_flick_element(image_client *client, ic_log *log, const char* zone, const char* element, int index, const char* direction);
		ic_log log = { 0 };
		ic_impl_flick_element(client, &log, zone, element, index, direction);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Force touch on element and drag for distance.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/ForceTouch </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_force_touch(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                           const char* zone /* Select Zone (default="") */,
	                           const char* element /* Select Element (default="") */,
	                           int index /* Element Order (default=0) */,
	                           int duration /* Duration (default=100) */,
	                           int force /* Force level in percent (default=100) */,
	                           int drag_distance_x /* Horizontal distance of drag from Element (default=0) */,
	                           int drag_distance_y /* Vertical distance of drag from Element (default=0) */,
	                           int drag_duration /* Drag Duration (default=1500) */)
	{
		IC_API void ic_impl_force_touch(image_client *client, ic_log *log, const char* zone, const char* element, int index, int duration, int force, int drag_distance_x, int drag_distance_y, int drag_duration);
		ic_log log = { 0 };
		ic_impl_force_touch(client, &log, zone, element, index, duration, force, drag_distance_x, drag_distance_y, drag_duration);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Get all the values of a property in a given element. Note: Supported properties are available on Object spy. 
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetAllValues </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_string_array ic_get_all_values(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                         const char* zone /* Select Zone (default="") */,
	                                         const char* element /* Select Element (default="") */,
	                                         const char* property /* Property (default="") */)
	{
		IC_API ic_string_array ic_impl_get_all_values(image_client *client, ic_log *log, const char* zone, const char* element, const char* property);
		ic_log log = { 0 };
		ic_string_array result = ic_impl_get_all_values(client, &log, zone, element, property);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get all the zones names that has an element with the given name.
	/// </summary>
	/// <returns> comma delimited string with the zones names </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_all_zones_with_element(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                                 const char* element /* the element name to search for (default="") */)
	{
		IC_API const char* ic_impl_get_all_zones_with_element(image_client *client, ic_log *log, const char* element);
		ic_log log = { 0 };
		const char* result = ic_impl_get_all_zones_with_element(client, &log, element);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Get available Agent Port.<br/>
	///	 			Get a port number of a SeeTest agent supporting (licensed to) the given<br/>
	///	 			device type.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_get_available_agent_port(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                       const char* feature_name /* A device type that is supported by the requested agent. One of ANDROID, IPHONE, BLACKBERRY or WINDOWS_PHONE. (default="") */)
	{
		IC_API int ic_impl_get_available_agent_port(image_client *client, ic_log *log, const char* feature_name);
		ic_log log = { 0 };
		int result = ic_impl_get_available_agent_port(client, &log, feature_name);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get an array of IDs of tabs in the opened browser (Chrome / Safari).
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_string_array ic_get_browser_tab_id_list(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API ic_string_array ic_impl_get_browser_tab_id_list(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_string_array result = ic_impl_get_browser_tab_id_list(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get connected devices.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_connected_devices(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_get_connected_devices(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_get_connected_devices(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	             Get the current context handlers list
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetContextList </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_string_array ic_get_context_list(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API ic_string_array ic_impl_get_context_list(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_string_array result = ic_impl_get_context_list(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Returns an integer representation in the RGB color model for coordinate (x,y) 
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetCoordinateColor </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_get_coordinate_color(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   int x /* the X coordinate of the target pixel (default=0) */,
	                                   int y /* the Y coordinate of the target pixel (default=0) */)
	{
		IC_API int ic_impl_get_coordinate_color(image_client *client, ic_log *log, int x, int y);
		ic_log log = { 0 };
		int result = ic_impl_get_coordinate_color(client, &log, x, y);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get monitor counter value
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetCounter </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_counter(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                  const char* counter_name /* Counter name (cpu, memory...) (default="") */)
	{
		IC_API const char* ic_impl_get_counter(image_client *client, ic_log *log, const char* counter_name);
		ic_log log = { 0 };
		const char* result = ic_impl_get_counter(client, &log, counter_name);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get the name of application that is running in the foreground of the device
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_current_application_name(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_get_current_application_name(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_get_current_application_name(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get the ID of the currently focused browser (Chrome / Safari) tab.
	/// </summary>
	/// <returns> ID of the current tab </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_current_browser_tab_id(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_get_current_browser_tab_id(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_get_current_browser_tab_id(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Gets the default timeout.
	/// </summary>
	/// <returns> the default timeout </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_get_default_timeout(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API int ic_impl_get_default_timeout(image_client *client, ic_log *log);
		ic_log log = { 0 };
		int result = ic_impl_get_default_timeout(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Download device log to reports directory
	/// </summary>
	/// <returns> path to the file that was downloaded </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_device_log(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_get_device_log(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_get_device_log(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	             Get device property value for the given key.
	/// </summary>
	/// <returns> The property value </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_device_property(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                          const char* key /* device_property (default="") */)
	{
		IC_API const char* ic_impl_get_device_property(image_client *client, ic_log *log, const char* key);
		ic_log log = { 0 };
		const char* result = ic_impl_get_device_property(client, &log, key);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get an XML formated string containing all the devices information
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_devices_information(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_get_devices_information(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_get_devices_information(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Count the number of times an element is been found in the current screen.
	/// </summary>
	/// <returns> the number of times the elment was identified </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetElementCount </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_get_element_count(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                const char* zone /* the element zone (default="") */,
	                                const char* element /* the element name (default="") */)
	{
		IC_API int ic_impl_get_element_count(image_client *client, ic_log *log, const char* zone, const char* element);
		ic_log log = { 0 };
		int result = ic_impl_get_element_count(client, &log, zone, element);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Search for an element and count the number of times an element is found near him.The direction can be UP, DOWN, LEFT and RIGHT.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_get_element_count_in(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* zone_name /* Select Zone (default="") */,
	                                   const char* element_search /* Element Search (default="") */,
	                                   int index /* Element index (default=0) */,
	                                   const char* direction /* Direction to analyze (default="") */,
	                                   const char* element_count_zone /* Select Zone (default="") */,
	                                   const char* element_count /* Element to count (default="") */,
	                                   int width /* Width of the search (0 indicate until the end/start of the window) (default=0) */,
	                                   int height /* Height of the search (0 indicate until the end/start of the window) (default=0) */)
	{
		IC_API int ic_impl_get_element_count_in(image_client *client, ic_log *log, const char* zone_name, const char* element_search, int index, const char* direction, const char* element_count_zone, const char* element_count, int width, int height);
		ic_log log = { 0 };
		int result = ic_impl_get_element_count_in(client, &log, zone_name, element_search, index, direction, element_count_zone, element_count, width, height);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get a string containing all installed application on the device
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_installed_applications(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_get_installed_applications(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_get_installed_applications(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Returns a CSV format of the running monitors (CPU/Memomy) for all<br/>
	///	       devices
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetMonitorsData </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_monitors_data(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        const char* c_s_vfilepath /* If set will save the CSV in the given location (should be absolute path), if not set will use a default location. (default="") */)
	{
		IC_API const char* ic_impl_get_monitors_data(image_client *client, ic_log *log, const char* c_s_vfilepath);
		ic_log log = { 0 };
		const char* result = ic_impl_get_monitors_data(client, &log, c_s_vfilepath);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			getting the network connection for a device.<br/>
	///	 			Execute any of the following connection type:<br/>
	///	 			airplane_mode, wifi, mobile_data.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_get_network_connection(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                         const char* connection /* Name of connection to get status (default="") */)
	{
		IC_API ic_bool ic_impl_get_network_connection(image_client *client, ic_log *log, const char* connection);
		ic_log log = { 0 };
		ic_bool result = ic_impl_get_network_connection(client, &log, connection);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get all Network Virtualization profiles.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetNVProfiles </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_string_array ic_get_n_v_profiles(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API ic_string_array ic_impl_get_n_v_profiles(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_string_array result = ic_impl_get_n_v_profiles(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get all values from picker, works only on iOS
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetPickerValues </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_string_array ic_get_picker_values(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                            const char* zone /* Select Zone (default="") */,
	                                            const char* picker_element /* Select Picker Element (default="") */,
	                                            int index /* Picker index (default=0) */,
	                                            int wheel_index /* Wheel index at picker component (default=0) */)
	{
		IC_API ic_string_array ic_impl_get_picker_values(image_client *client, ic_log *log, const char* zone, const char* picker_element, int index, int wheel_index);
		ic_log log = { 0 };
		ic_string_array result = ic_impl_get_picker_values(client, &log, zone, picker_element, index, wheel_index);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Get SeeTest's property value for the given key (in %appdata%\seetest\app.properties file).
	/// </summary>
	/// <returns> the property value </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_property(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* property /* The property key. (default="") */)
	{
		IC_API const char* ic_impl_get_property(image_client *client, ic_log *log, const char* property);
		ic_log log = { 0 };
		const char* result = ic_impl_get_property(client, &log, property);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get sim-card name assigned to the currently used device
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_sim_card(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_get_sim_card(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_get_sim_card(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get list of sim-card names which are either ready to use or already assigned (depeand on the argument) to the currently used device
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_string_array ic_get_sim_cards(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        ic_bool ready_to_use /* If TRUE return all available sim cards which are ready to use. If FALSE return all sim cards assigned for the device (default=IC_TRUE) */)
	{
		IC_API ic_string_array ic_impl_get_sim_cards(image_client *client, ic_log *log, ic_bool ready_to_use);
		ic_log log = { 0 };
		ic_string_array result = ic_impl_get_sim_cards(client, &log, ready_to_use);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get text content of table cell by its header element and row element.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_table_cell_text(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                          const char* zone /* Select zone (default="") */,
	                                          const char* header_element /* Select table header element (default="") */,
	                                          int header_index /* Header element index (default=0) */,
	                                          const char* row_element /* Select table row element (default="") */,
	                                          int row_index /* Row element index (default=0) */,
	                                          int width /* Width of the search (default=0) */,
	                                          int height /* Height of the search (default=0) */)
	{
		IC_API const char* ic_impl_get_table_cell_text(image_client *client, ic_log *log, const char* zone, const char* header_element, int header_index, const char* row_element, int row_index, int width, int height);
		ic_log log = { 0 };
		const char* result = ic_impl_get_table_cell_text(client, &log, zone, header_element, header_index, row_element, row_index, width, height);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Gets the text that appears in a specified zone.
	/// </summary>
	/// <returns> the text </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetText </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_text(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                               const char* zone /* Select the Zone to Get Text From, 'TEXT' and 'NATIVE' can be used as well. (default="") */)
	{
		IC_API const char* ic_impl_get_text(image_client *client, ic_log *log, const char* zone);
		ic_log log = { 0 };
		const char* result = ic_impl_get_text(client, &log, zone);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get the text in a specific area relative to an element, index, direction, width and height. Direction can be UP, DOWN, LEFT and RIGHT.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetTextIn </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_text_in(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                  const char* zone /* Select Zone (default="") */,
	                                  const char* element /* Select Element (default="") */,
	                                  int index /* Element index (default=0) */,
	                                  const char* text_zone /* The zone to extract the text from (default="") */,
	                                  const char* direction /* Direction to analyze (default="") */,
	                                  int width /* Width of the search (0 indicate until the end/start of the window) (default=0) */,
	                                  int height /* Height of the search (0 indicate until the end/start of the window) (default=0) */,
	                                  int x_offset /* identification rectangle x offset (default=0) */,
	                                  int y_offset /* identification rectangle y offset (default=0) */)
	{
		IC_API const char* ic_impl_get_text_in(image_client *client, ic_log *log, const char* zone, const char* element, int index, const char* text_zone, const char* direction, int width, int height, int x_offset, int y_offset);
		ic_log log = { 0 };
		const char* result = ic_impl_get_text_in(client, &log, zone, element, index, text_zone, direction, width, height, x_offset, y_offset);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get visual dump
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/GetVisualDump </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_get_visual_dump(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* type /* Set the dump type (default="Native") */)
	{
		IC_API const char* ic_impl_get_visual_dump(image_client *client, ic_log *log, const char* type);
		ic_log log = { 0 };
		const char* result = ic_impl_get_visual_dump(client, &log, type);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Clear browser cookies and/or cache
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_hybrid_clear_cache(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                  ic_bool clear_cookies /* If true cookies will be cleared (default=IC_TRUE) */,
	                                  ic_bool clear_cache /* If true cache will be cleared (default=IC_TRUE) */)
	{
		IC_API void ic_impl_hybrid_clear_cache(image_client *client, ic_log *log, ic_bool clear_cookies, ic_bool clear_cache);
		ic_log log = { 0 };
		ic_impl_hybrid_clear_cache(client, &log, clear_cookies, clear_cache);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Get HTML content from a webView element
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_hybrid_get_html(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* web_view_locator /* WebView locator string like id=web or empty for the first WebView in page (default="") */,
	                                      int index /* Element index (default=0) */)
	{
		IC_API const char* ic_impl_hybrid_get_html(image_client *client, ic_log *log, const char* web_view_locator, int index);
		ic_log log = { 0 };
		const char* result = ic_impl_hybrid_get_html(client, &log, web_view_locator, index);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Run Javascript in a WebView.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/HybridRunJavaScript </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_hybrid_run_javascript(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                            const char* web_view_locator /* WebView locator string like id=web or empty for the first WebView in page (default="") */,
	                                            int index /* Element index (default=0) */,
	                                            const char* script /* Javascript (default="") */)
	{
		IC_API const char* ic_impl_hybrid_run_javascript(image_client *client, ic_log *log, const char* web_view_locator, int index, const char* script);
		ic_log log = { 0 };
		const char* result = ic_impl_hybrid_run_javascript(client, &log, web_view_locator, index, script);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Select an option from a select element in a WebView. Using the input method and value to identify the element
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/HybridSelect </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_hybrid_select(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                             const char* web_view_locator /* WebView locator string like id=web or empty for the first WebView in page (default="") */,
	                             int index /* Element index (default=0) */,
	                             const char* method /* Identification method (css/id) (default="") */,
	                             const char* value /* Identification value (default="") */,
	                             const char* select /* Option to select (default="") */)
	{
		IC_API void ic_impl_hybrid_select(image_client *client, ic_log *log, const char* web_view_locator, int index, const char* method, const char* value, const char* select);
		ic_log log = { 0 };
		ic_impl_hybrid_select(client, &log, web_view_locator, index, method, value, select);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Wait for web page to load
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/HybridWaitForPageLoad </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_hybrid_wait_for_page_load(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                         int timeout /* Waiting Timeout in MiliSec (default=10000) */)
	{
		IC_API void ic_impl_hybrid_wait_for_page_load(image_client *client, ic_log *log, int timeout);
		ic_log log = { 0 };
		ic_impl_hybrid_wait_for_page_load(client, &log, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Install the application in the given path on the device
	/// </summary>
	/// <returns> installation success </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/Install </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_install(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                          const char* path /* can be an APK/IPA etc. absolute path or an activity name from the application manager. (default="") */,
	                          ic_bool instrument /* If set to TRUE will sign the application (if not already instrumented) (default=IC_TRUE) */,
	                          ic_bool keep_data /* If set to TRUE will keep application data (default=IC_FALSE) */)
	{
		IC_API ic_bool ic_impl_install(image_client *client, ic_log *log, const char* path, ic_bool instrument, ic_bool keep_data);
		ic_log log = { 0 };
		ic_bool result = ic_impl_install(client, &log, path, instrument, keep_data);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Install instrumented application Signed with a given keystore
	/// </summary>
	/// <returns> installation success </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_install_with_custom_keystore(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                               const char* path /* can be an APK/IPA etc. absolute path or an activity name from the application manager. (default="") */,
	                                               ic_bool instrument /* If set to TRUE will sign the application (if not already instrumented) (default=IC_TRUE) */,
	                                               ic_bool keep_data /* If set to TRUE will keep application data (default=IC_FALSE) */,
	                                               const char* keystore_path /* Key-store file location on Disk (default="") */,
	                                               const char* keystore_password /* The key-store password (default="") */,
	                                               const char* key_alias /* The key alias (default="") */,
	                                               const char* key_password /* The key password (default="") */)
	{
		IC_API ic_bool ic_impl_install_with_custom_keystore(image_client *client, ic_log *log, const char* path, ic_bool instrument, ic_bool keep_data, const char* keystore_path, const char* keystore_password, const char* key_alias, const char* key_password);
		ic_log log = { 0 };
		ic_bool result = ic_impl_install_with_custom_keystore(client, &log, path, instrument, keep_data, keystore_path, keystore_password, key_alias, key_password);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Check if a given element found in the specified zone is blank; if blank returns TRUE if not found returns FALSE
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/IsElementBlank </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_is_element_blank(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* zone /* Select Zone (default="") */,
	                                   const char* element /* Element to find (default="") */,
	                                   int index /* Element index (default=0) */,
	                                   int color_groups /* The number of color groups that indicate an image (default=10) */)
	{
		IC_API ic_bool ic_impl_is_element_blank(image_client *client, ic_log *log, const char* zone, const char* element, int index, int color_groups);
		ic_log log = { 0 };
		ic_bool result = ic_impl_is_element_blank(client, &log, zone, element, index, color_groups);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Check if a given element is found in the specified zone; if found returns TRUE if not found returns FALSE
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/IsElementFound </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_is_element_found(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* zone /* Select Zone (default="") */,
	                                   const char* element /* Element to find (default="") */,
	                                   int index /* Element index (default=0) */)
	{
		IC_API ic_bool ic_impl_is_element_found(image_client *client, ic_log *log, const char* zone, const char* element, int index);
		ic_log log = { 0 };
		ic_bool result = ic_impl_is_element_found(client, &log, zone, element, index);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Search for an element and check if an element related to it exist. The direction can be UP, DOWN, LEFT and RIGHT.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/IsFoundIn </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_is_found_in(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                              const char* zone /* Select Zone (default="") */,
	                              const char* search_element /* Search Element (default="") */,
	                              int index /* Element index (default=0) */,
	                              const char* direction /* Direction to analyze (default="") */,
	                              const char* element_find_zone /* Find Element Zone (default="") */,
	                              const char* element_to_find /* Element to Find (default="") */,
	                              int width /* Width of the search (0 indicate until the end/start of the window) (default=0) */,
	                              int height /* Height of the search (0 indicate until the end/start of the window) (default=0) */)
	{
		IC_API ic_bool ic_impl_is_found_in(image_client *client, ic_log *log, const char* zone, const char* search_element, int index, const char* direction, const char* element_find_zone, const char* element_to_find, int width, int height);
		ic_log log = { 0 };
		ic_bool result = ic_impl_is_found_in(client, &log, zone, search_element, index, direction, element_find_zone, element_to_find, width, height);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Launch activity
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Launch </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_launch(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                      const char* activity_u_r_l /* The application main activity or URL (default="") */,
	                      ic_bool instrument /* If set to true then will launch in instrument mode (default=IC_TRUE) */,
	                      ic_bool stop_if_running /* If set to true then will stop the running process before launching it (default=IC_FALSE) */)
	{
		IC_API void ic_impl_launch(image_client *client, ic_log *log, const char* activity_u_r_l, ic_bool instrument, ic_bool stop_if_running);
		ic_log log = { 0 };
		ic_impl_launch(client, &log, activity_u_r_l, instrument, stop_if_running);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Select an element from a list. Used for non-touch devices like blackberry.
	/// </summary>
	/// <returns> true if element was found </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/ListSelect </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_list_select(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                              const char* send_rest /* Navigate to the list start (default="{UP}") */,
	                              const char* send_navigation /* Send to navigate in the list (default="{DOWN}") */,
	                              int delay /* Time to wait before sending a command (in MiliSec) (default=500) */,
	                              const char* text_to_identify /* Select text to Find (default="") */,
	                              const char* color /* Color to filter (default="") */,
	                              int rounds /* Maximum navigation rounds (default=5) */,
	                              const char* sendonfind /* Send on text find (default="{ENTER}") */)
	{
		IC_API ic_bool ic_impl_list_select(image_client *client, ic_log *log, const char* send_rest, const char* send_navigation, int delay, const char* text_to_identify, const char* color, int rounds, const char* sendonfind);
		ic_log log = { 0 };
		ic_bool result = ic_impl_list_select(client, &log, send_rest, send_navigation, delay, text_to_identify, color, rounds, sendonfind);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Long click on or near to an element (the proximity to the element is specified by a X-Y offset).
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/LongClick </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_long_click(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                          const char* zone /* Select Zone (default="") */,
	                          const char* element /* Select Element (default="") */,
	                          int index /* Element Order (default=0) */,
	                          int click_count /* Number of Clicks (default=1) */,
	                          int x /* Horizontal Offset from Element (default=0) */,
	                          int y /* Vertical Offset from Element (default=0) */)
	{
		IC_API void ic_impl_long_click(image_client *client, ic_log *log, const char* zone, const char* element, int index, int click_count, int x, int y);
		ic_log log = { 0 };
		ic_impl_long_click(client, &log, zone, element, index, click_count, x, y);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Click on an element with a finger.<br/>
	///	 Note: element will be identified right before performing the gesutre
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiClick </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_click(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                           const char* zone /* Select Zone (default="") */,
	                           const char* element /* Select Element (default="") */,
	                           int index /* Element index (default=0) */,
	                           int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_click(image_client *client, ic_log *log, const char* zone, const char* element, int index, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_click(client, &log, zone, element, index, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Click with a finger on given coordinate
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiClickCoordinate </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_click_coordinate(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      int x /* Horizontal coordinate */,
	                                      int y /* Vertical coordinate */,
	                                      int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_click_coordinate(image_client *client, ic_log *log, int x, int y, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_click_coordinate(client, &log, x, y, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Click with a finger near an element, in given offset form the element.<br/>
	///	 Note: element will be identified right before performing the gesutre
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiClickOffset </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_click_offset(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                  const char* zone /* Select Zone (default="") */,
	                                  const char* element /* Select Element (default="") */,
	                                  int index /* Element index (default=0) */,
	                                  int x /* Horizontal coordinate */,
	                                  int y /* Vertical coordinate */,
	                                  int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_click_offset(image_client *client, ic_log *log, const char* zone, const char* element, int index, int x, int y, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_click_offset(client, &log, zone, element, index, x, y, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Swipe the screen in a given direction
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiSwipe </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_swipe(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                           const char* direction /* Swipe direction (default="") */,
	                           int offset /* Swipe offset */,
	                           int time /* Swipe overall time (default=500) */,
	                           int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_swipe(image_client *client, ic_log *log, const char* direction, int offset, int time, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_swipe(client, &log, direction, offset, time, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Hold a finger Touched down on an element. Release with MultiTouchUp command.<br/>
	///	 Note: element will be identified right before performing the gesture
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiTouchDown </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_touch_down(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                const char* zone /* Select Zone (default="") */,
	                                const char* element /* Select Element (default="") */,
	                                int index /* Element index (default=0) */,
	                                int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_touch_down(image_client *client, ic_log *log, const char* zone, const char* element, int index, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_touch_down(client, &log, zone, element, index, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Hold a finger Touched down on a given coordinate
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiTouchDownCoordinate </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_touch_down_coordinate(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           int x /* Horizontal coordinate */,
	                                           int y /* Vertical coordinate */,
	                                           int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_touch_down_coordinate(image_client *client, ic_log *log, int x, int y, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_touch_down_coordinate(client, &log, x, y, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Gesture-move a finger to the element, from the last coordinate touched down or moved to.<br/>
	///	 Note: element will be identified right before performing the gesutre
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiTouchDown </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_touch_move(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                const char* zone /* Select Zone (default="") */,
	                                const char* element /* Select Element (default="") */,
	                                int index /* Element index (default=0) */,
	                                int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_touch_move(image_client *client, ic_log *log, const char* zone, const char* element, int index, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_touch_move(client, &log, zone, element, index, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Move a finger to the given coordinate, from the last coordinate touched down or moved to.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiTouchMoveCoordinate </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_touch_move_coordinate(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           int x /* Horizontal coordinate */,
	                                           int y /* Vertical coordinate */,
	                                           int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_touch_move_coordinate(image_client *client, ic_log *log, int x, int y, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_touch_move_coordinate(client, &log, x, y, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Touch up with a finger, from the last coordinate touched down or moved to
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiTouchUp </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_touch_up(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                              int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_touch_up(image_client *client, ic_log *log, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_touch_up(client, &log, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Wait with a finger in the last touch point, for the given time
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/MultiWait </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_multi_wait(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                          int time /* Wait time (default=500) */,
	                          int finger_index /* Finger index */)
	{
		IC_API void ic_impl_multi_wait(image_client *client, ic_log *log, int time, int finger_index);
		ic_log log = { 0 };
		ic_impl_multi_wait(client, &log, time, finger_index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Opens current device's screen.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_open_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_open_device(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_open_device(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Convert percentage into pixel.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_p2cx(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                   int percentage /* Screen Percentage (default=0) */)
	{
		IC_API int ic_impl_p2cx(image_client *client, ic_log *log, int percentage);
		ic_log log = { 0 };
		int result = ic_impl_p2cx(client, &log, percentage);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Convert percentage into pixel.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_p2cy(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                   int percentage /* Screen Percentage (default=0) */)
	{
		IC_API int ic_impl_p2cy(image_client *client, ic_log *log, int percentage);
		ic_log log = { 0 };
		int result = ic_impl_p2cy(client, &log, percentage);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Perform the multi-touch gesture steps which were added after the last StartMultiGestureStep command
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/PerformMultiGestureStep </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_perform_multi_gesture(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_perform_multi_gesture(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_perform_multi_gesture(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Pinch In/Out at X,Y in specific radius, if X and Y equal to 0, the pinch will be performed from the center of the screen. In most cases, pinching IN would be used to ZOOM OUT, and pinching OUT would be used to ZOOM IN.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Pinch </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_pinch(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                        ic_bool inside /* Set 'true' to pinch in (zoom out), 'false' to pinch out (zoom in)  (default=IC_TRUE) */,
	                        int x /* X center (default=0) */,
	                        int y /* Y center (default=0) */,
	                        int radius /* The pinch radius (default=100) */)
	{
		IC_API ic_bool ic_impl_pinch(image_client *client, ic_log *log, ic_bool inside, int x, int y, int radius);
		ic_log log = { 0 };
		ic_bool result = ic_impl_pinch(client, &log, inside, x, y, radius);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Pinch In/Out at X,Y in specific radius, if X and Y equal to 0, the pinch will be performed from the center of the screen. In most cases, pinching IN would be used to ZOOM OUT, and pinching OUT would be used to ZOOM IN.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Pinch </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_pinch2(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                         ic_bool inside /* Set 'true' to pinch in (zoom out), 'false' to pinch out (zoom in)  (default=IC_TRUE) */,
	                         int x /* X center (default=0) */,
	                         int y /* Y center (default=0) */,
	                         int radius /* The pinch radius (default=100) */,
	                         ic_bool horizontal /* Vertical / horizontal pinch (default=IC_FALSE) */)
	{
		IC_API ic_bool ic_impl_pinch2(image_client *client, ic_log *log, ic_bool inside, int x, int y, int radius, ic_bool horizontal);
		ic_log log = { 0 };
		ic_bool result = ic_impl_pinch2(client, &log, inside, x, y, radius, horizontal);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			ping Server.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_ping_server(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_ping_server(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_ping_server(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Forward an agent port
	/// </summary>
	/// <returns> the remote port assigned </returns>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static int ic_port_forward(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                           int local_port /* The local port */,
	                           int remote_port /* The remote port */)
	{
		IC_API int ic_impl_port_forward(image_client *client, ic_log *log, int local_port, int remote_port);
		ic_log log = { 0 };
		int result = ic_impl_port_forward(client, &log, local_port, remote_port);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Press on a certain element (ElementToClick) while another element (ElementToFind)is not found.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/PressWhileNotFound </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_press_while_not_found(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                     const char* zone /* Select Zone (default="") */,
	                                     const char* elementtoclick /* Select Element To Click (default="") */,
	                                     int elementtoclickindex /* Element To Click Index (default=0) */,
	                                     const char* elementtofind /* Select Element To Find (default="") */,
	                                     int elementtofindindex /* Element To Find Index (default=0) */,
	                                     int timeout /* Waiting Timeout in MiliSec (default=10000) */,
	                                     int delay /* Time to wait before clicking on the ElementToClick (in MiliSec) (default=0) */)
	{
		IC_API void ic_impl_press_while_not_found(image_client *client, ic_log *log, const char* zone, const char* elementtoclick, int elementtoclickindex, const char* elementtofind, int elementtofindindex, int timeout, int delay);
		ic_log log = { 0 };
		ic_impl_press_while_not_found(client, &log, zone, elementtoclick, elementtoclickindex, elementtofind, elementtofindindex, timeout, delay);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Will reboot the device
	/// </summary>
	/// <returns> have device already been rebooted within given timeout </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/Reboot </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_reboot(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                         int timeout /* Timeout waiting for the device to reboot. Minimum value is 40000. (default=120000) */)
	{
		IC_API ic_bool ic_impl_reboot(image_client *client, ic_log *log, int timeout);
		ic_log log = { 0 };
		ic_bool result = ic_impl_reboot(client, &log, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Initiate a 3rd party phone call to be received by the device.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_receive_incoming_call(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                     const char* from_number /* Number that the call will come from (default="") */,
	                                     int hangup_in_seconds /* How long to wait before hanging up while ringing */)
	{
		IC_API void ic_impl_receive_incoming_call(image_client *client, ic_log *log, const char* from_number, int hangup_in_seconds);
		ic_log log = { 0 };
		ic_impl_receive_incoming_call(client, &log, from_number, hangup_in_seconds);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Initiate a 3rd party SMS to be received by the device.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_receive_incoming_s_m_s(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* from_number /* Number that the SMS will come from (default="") */,
	                                      const char* msg /* Text of the message to receive (default="") */)
	{
		IC_API void ic_impl_receive_incoming_s_m_s(image_client *client, ic_log *log, const char* from_number, const char* msg);
		ic_log log = { 0 };
		ic_impl_receive_incoming_s_m_s(client, &log, from_number, msg);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Release device as well as the agent assigned to it. It will enable other tests that are waiting to be executed to start doing so.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_release_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                              const char* device_name /* Name of the device to release (default="") */,
	                              ic_bool release_agent /* Deprecated. If TRUE or FALSE then agent will be released (default=IC_TRUE) */,
	                              ic_bool remove_from_device_list /* If TRUE then the device will be remove from the device list (default=IC_FALSE) */,
	                              ic_bool release_from_cloud /* If TRUE then the device will be released from the cloud (default=IC_TRUE) */)
	{
		IC_API void ic_impl_release_device(image_client *client, ic_log *log, const char* device_name, ic_bool release_agent, ic_bool remove_from_device_list, ic_bool release_from_cloud);
		ic_log log = { 0 };
		ic_impl_release_device(client, &log, device_name, release_agent, remove_from_device_list, release_from_cloud);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Add a step with a custom message to the test report.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_report(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                      const char* message /* The message to report. (default="") */,
	                      ic_bool status /* TRUE marks this step as successfully passed, FALSE marks it as failed. */)
	{
		IC_API void ic_impl_report(image_client *client, ic_log *log, const char* message, ic_bool status);
		ic_log log = { 0 };
		ic_impl_report(client, &log, message, status);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Add a step with a custom message and image to the test report.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Report </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_report_with_image(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                 const char* path_to_image /* Pass to the image, can be either a local full path or a URL. (default="") */,
	                                 const char* message /* The message to report. (default="") */,
	                                 ic_bool status /* TRUE marks this step as successfully passed, FALSE marks it as failed. */)
	{
		IC_API void ic_impl_report_with_image(image_client *client, ic_log *log, const char* path_to_image, const char* message, ic_bool status);
		ic_log log = { 0 };
		ic_impl_report_with_image(client, &log, path_to_image, message, status);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Reset Device Bridge.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_reset_device_bridge(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_reset_device_bridge(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_reset_device_bridge(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Reset Device Bridge.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_reset_device_bridge_o_s(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                       const char* device_type /* A device type that is supported by the requested agent (default="") */)
	{
		IC_API void ic_impl_reset_device_bridge_o_s(image_client *client, ic_log *log, const char* device_type);
		ic_log log = { 0 };
		ic_impl_reset_device_bridge_o_s(client, &log, device_type);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Right-click on or near to an element (the proximity to the element is specified by a X-Y offset)
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_right_click(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                           const char* zone /* Select Zone (default="") */,
	                           const char* element /* Select Element (default="") */,
	                           int index /* Element Order (default=0) */,
	                           int click_count /* Number of Clicks (default=1) */,
	                           int x /* Horizontal Offset from Element (default=0) */,
	                           int y /* Vertical Offset from Element (default=0) */)
	{
		IC_API void ic_impl_right_click(image_client *client, ic_log *log, const char* zone, const char* element, int index, int click_count, int x, int y);
		ic_log log = { 0 };
		ic_impl_right_click(client, &log, zone, element, index, click_count, x, y);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Run the given command line as shell command on the device (adb for android and SSH for iOS).<br/>
	///	 			When running an executable a full path	should be given.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Run </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_run(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                          const char* command /* The command to run (default="") */)
	{
		IC_API const char* ic_impl_run(image_client *client, ic_log *log, const char* command);
		ic_log log = { 0 };
		const char* result = ic_impl_run(client, &log, command);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Run a layout test
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_layout_test(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                  const char* xml /* the xml of the layout tests (default="") */)
	{
		IC_API const char* ic_impl_layout_test(image_client *client, ic_log *log, const char* xml);
		ic_log log = { 0 };
		const char* result = ic_impl_layout_test(client, &log, xml);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Run native API call on the given element. 
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_run_native_a_p_i_call(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                            const char* zone /* Select Zone (should be NATIVE) (default="") */,
	                                            const char* element /* Select Element (should use xpath) (default="") */,
	                                            int index /* Element index (default=0) */,
	                                            const char* script /* Script to execute (default="") */)
	{
		IC_API const char* ic_impl_run_native_a_p_i_call(image_client *client, ic_log *log, const char* zone, const char* element, int index, const char* script);
		ic_log log = { 0 };
		const char* result = ic_impl_run_native_a_p_i_call(client, &log, zone, element, index, script);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Sends/inserts text to the application. Supports the following keyboard actions as well: {ENTER}, {BKSP}. Device actions can now be found under DeviceAction command.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SendText </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_send_text(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                         const char* text /* Text to send (default="") */)
	{
		IC_API void ic_impl_send_text(image_client *client, ic_log *log, const char* text);
		ic_log log = { 0 };
		ic_impl_send_text(client, &log, text);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Send a given text while an element is not found
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SendWhileNotFound </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_send_while_not_found(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                    const char* to_send /* Insert Text To Send (default="") */,
	                                    const char* zone /* Select Zone (default="") */,
	                                    const char* elementtofind /* Select Element to Find (default="") */,
	                                    int elementtofindindex /* Element to Find Index (default=0) */,
	                                    int timeout /* Waiting Timeout in MiliSec (default=10000) */,
	                                    int delay /* Time to wait before sending a command (in MiliSec) (default=1000) */)
	{
		IC_API void ic_impl_send_while_not_found(image_client *client, ic_log *log, const char* to_send, const char* zone, const char* elementtofind, int elementtofindindex, int timeout, int delay);
		ic_log log = { 0 };
		ic_impl_send_while_not_found(client, &log, to_send, zone, elementtofind, elementtofindindex, timeout, delay);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Sets the application title.<br/>
	///	 	 		Switch tested application/window.<br/>
	///	 	 		The title of the application is searched in the following order:<br/>
	///	 	 		1. search for windows with the given window ID (only if the title is a number).<br/>
	///	 	 		2. search for complete match.<br/>
	///	 	 		3. search for partial match (contains).<br/>
	///	 	 		4. Regular expression match.<br/>
	///	 	 		When working on the desktop use the 'desktop' as the application title.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_application_title(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                     const char* title /* Insert New Application/Window Title (default="") */)
	{
		IC_API void ic_impl_set_application_title(image_client *client, ic_log *log, const char* title);
		ic_log log = { 0 };
		ic_impl_set_application_title(client, &log, title);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Set the reply type for later authentication requests.<br/>
	///	 See online documentation for details.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_authentication_reply(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        const char* reply /* The reply type to mock (default="Success") */,
	                                        int delay /* Delay after request in ms (default=0) */)
	{
		IC_API void ic_impl_set_authentication_reply(image_client *client, ic_log *log, const char* reply, int delay);
		ic_log log = { 0 };
		ic_impl_set_authentication_reply(client, &log, reply, delay);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Set the current context
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetContext </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_context(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                           const char* context /* Context (default="NATIVE_APP") */)
	{
		IC_API void ic_impl_set_context(image_client *client, ic_log *log, const char* context);
		ic_log log = { 0 };
		ic_impl_set_context(client, &log, context);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set the default click down time in milliseconds (default is 100)
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetDefaultClickDownTime </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_default_click_down_time(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                           int down_time /* Click down time value (default=100) */)
	{
		IC_API void ic_impl_set_default_click_down_time(image_client *client, ic_log *log, int down_time);
		ic_log log = { 0 };
		ic_impl_set_default_click_down_time(client, &log, down_time);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set the default timeout for click commands
	/// </summary>
	/// <returns> the new timeout </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetDefaultTimeout </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_set_default_timeout(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                          int new_timeout /* Waiting Timeout in MiliSec (default=20000) */)
	{
		IC_API const char* ic_impl_set_default_timeout(image_client *client, ic_log *log, int new_timeout);
		ic_log log = { 0 };
		const char* result = ic_impl_set_default_timeout(client, &log, new_timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Set the WebView to work with
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_default_web_view(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                    const char* web_view_locator /* The locator of the WebView, empty to reset (default="") */)
	{
		IC_API void ic_impl_set_default_web_view(image_client *client, ic_log *log, const char* web_view_locator);
		ic_log log = { 0 };
		ic_impl_set_default_web_view(client, &log, web_view_locator);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Sets the device.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                          const char* device /* Device name (default="") */)
	{
		IC_API void ic_impl_set_device(image_client *client, ic_log *log, const char* device);
		ic_log log = { 0 };
		ic_impl_set_device(client, &log, device);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Drag called after setting dragStartDelay will begin by long touch on the device in<br/>
	///	 			the initial drag location.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetDragStartDelay </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_drag_start_delay(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                    int delay /* Delay value (default=0) */)
	{
		IC_API void ic_impl_set_drag_start_delay(image_client *client, ic_log *log, int delay);
		ic_log log = { 0 };
		ic_impl_set_drag_start_delay(client, &log, delay);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set the time between key down and key up
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetInKeyDelay </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_in_key_delay(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                int delay /* Delay value (default=50) */)
	{
		IC_API void ic_impl_set_in_key_delay(image_client *client, ic_log *log, int delay);
		ic_log log = { 0 };
		ic_impl_set_in_key_delay(client, &log, delay);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set the time between two key press
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetKeyToKeyDelay </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_key_to_key_delay(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                    int delay /* Delay value (default=50) */)
	{
		IC_API void ic_impl_set_key_to_key_delay(image_client *client, ic_log *log, int delay);
		ic_log log = { 0 };
		ic_impl_set_key_to_key_delay(client, &log, delay);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set the OCR language
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetLanguage </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_language(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                            const char* language /* The language to use or empty to reset to English (default="") */)
	{
		IC_API void ic_impl_set_language(image_client *client, ic_log *log, const char* language);
		ic_log log = { 0 };
		ic_impl_set_language(client, &log, language);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set language properties file
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_language_properties_file(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                            const char* propertiesfile /* The language properties file path (default="") */)
	{
		IC_API void ic_impl_set_language_properties_file(image_client *client, ic_log *log, const char* propertiesfile);
		ic_log log = { 0 };
		ic_impl_set_language_properties_file(client, &log, propertiesfile);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Sets device's location to specified coordinate. Currently supported only on android
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_location(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                            const char* latitude /* Latitude in decimal degrees from -90 to 90. Positive latitudes are north of the equator and negative latitudes are south of the equator. (default="0.0") */,
	                            const char* longitude /* Longitude in decimal degrees from -180 to 180. Positive longitudes are east of the Prime Meridian and negative longitudes are west of the Prime Meridian (default="0.0") */)
	{
		IC_API void ic_impl_set_location(image_client *client, ic_log *log, const char* latitude, const char* longitude);
		ic_log log = { 0 };
		ic_impl_set_location(client, &log, latitude, longitude);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set monitor polling interval
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_monitor_polling_interval(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                            int timemilli /* The monitor polling time interval in milliseconds (default=30000) */)
	{
		IC_API void ic_impl_set_monitor_polling_interval(image_client *client, ic_log *log, int timemilli);
		ic_log log = { 0 };
		ic_impl_set_monitor_polling_interval(client, &log, timemilli);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set the test status to the monitors
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_monitor_test_state(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* test_status /* Set the current status of the test to be used by the monitors (default="") */)
	{
		IC_API void ic_impl_set_monitor_test_state(image_client *client, ic_log *log, const char* test_status);
		ic_log log = { 0 };
		ic_impl_set_monitor_test_state(client, &log, test_status);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set profile for current device. To cancel profile leave the profile name empty
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_network_conditions(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* profile /* Set the profile to be used (default="") */)
	{
		IC_API void ic_impl_set_network_conditions(image_client *client, ic_log *log, const char* profile);
		ic_log log = { 0 };
		ic_impl_set_network_conditions(client, &log, profile);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set profile for current device for a specified duration. To cancel profile leave the profile name empty
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_network_conditions2(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                       const char* profile /* Set the profile to be used (default="") */,
	                                       int duration /* Set the duration in milliseconds to which to set the profile (0=forever) (default=0) */)
	{
		IC_API void ic_impl_set_network_conditions2(image_client *client, ic_log *log, const char* profile, int duration);
		ic_log log = { 0 };
		ic_impl_set_network_conditions2(client, &log, profile, duration);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			setting the network connection for a device.<br/>
	///	 			Execute any of the following connection type:<br/>
	///	 			airplane_mode, wifi, mobile_data.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_network_connection(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* connection /* Name of connection to set (default="") */,
	                                      ic_bool enable /* Status of connection to set */)
	{
		IC_API void ic_impl_set_network_connection(image_client *client, ic_log *log, const char* connection, ic_bool enable);
		ic_log log = { 0 };
		ic_impl_set_network_connection(client, &log, connection, enable);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Set the ignore case status (default is true).
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_ocr_ignore_case(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   ic_bool ignore_case /* should OCR avoid case sensitivity */)
	{
		IC_API void ic_impl_set_ocr_ignore_case(image_client *client, ic_log *log, ic_bool ignore_case);
		ic_log log = { 0 };
		ic_impl_set_ocr_ignore_case(client, &log, ignore_case);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_ocr_training_file_path(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                          const char* training_path /* path to the OCR training file (default="") */)
	{
		IC_API void ic_impl_set_ocr_training_file_path(image_client *client, ic_log *log, const char* training_path);
		ic_log log = { 0 };
		ic_impl_set_ocr_training_file_path(client, &log, training_path);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set the values of the Picker element
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetPickerValues </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_set_picker_values(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        const char* zone_name /* Select Zone (default="") */,
	                                        const char* element_name /* Select Element (default="") */,
	                                        int index /* Element index (default=0) */,
	                                        int wheel_index /* Wheel Index */,
	                                        const char* value /* The value to set (default="") */)
	{
		IC_API const char* ic_impl_set_picker_values(image_client *client, ic_log *log, const char* zone_name, const char* element_name, int index, int wheel_index, const char* value);
		ic_log log = { 0 };
		const char* result = ic_impl_set_picker_values(client, &log, zone_name, element_name, index, wheel_index, value);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Sets the project base directory.<br/>
	///	 			This method should be called first,	before any other called is used.<br/>
	///	 			In case of working on a remote agent, the project path should be the path on the remote machine.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetProjectBaseDirectory </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_project_base_directory(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                          const char* project_base_directory /* the new project base directory. (default="") */)
	{
		IC_API void ic_impl_set_project_base_directory(image_client *client, ic_log *log, const char* project_base_directory);
		ic_log log = { 0 };
		ic_impl_set_project_base_directory(client, &log, project_base_directory);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetProperty </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_property(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                            const char* key /* key (default="") */,
	                            const char* value /* Value (default="") */)
	{
		IC_API void ic_impl_set_property(image_client *client, ic_log *log, const char* key, const char* value);
		ic_log log = { 0 };
		ic_impl_set_property(client, &log, key, value);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_red_to_blue(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                               ic_bool red_to_blue /*  */)
	{
		IC_API void ic_impl_set_red_to_blue(image_client *client, ic_log *log, ic_bool red_to_blue);
		ic_log log = { 0 };
		ic_impl_set_red_to_blue(client, &log, red_to_blue);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Sets the reporter. Configure the internal reporter.
	/// </summary>
	/// <returns> The reports directory path </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetReporter </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_set_reporter(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* reporter_name /* Comma seperated value string of reporter types. Supported types: html(=xml), pdf. (default="html") */,
	                                   const char* directory /* The directory for the report to be generated in. (default="") */,
	                                   const char* test_name /* The name of the test as would appear in the report. (default="") */)
	{
		IC_API const char* ic_impl_set_reporter(image_client *client, ic_log *log, const char* reporter_name, const char* directory, const char* test_name);
		ic_log log = { 0 };
		const char* result = ic_impl_set_reporter(client, &log, reporter_name, directory, test_name);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Show report images as link.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_show_image_as_link(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      ic_bool show_image_as_link /* show report images as link */)
	{
		IC_API void ic_impl_set_show_image_as_link(image_client *client, ic_log *log, ic_bool show_image_as_link);
		ic_log log = { 0 };
		ic_impl_set_show_image_as_link(client, &log, show_image_as_link);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Decide whether to include step screenshots in report.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_show_image_in_report(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        ic_bool show_image_in_report /* when set to False will not show any screenshot in report (default=IC_TRUE) */)
	{
		IC_API void ic_impl_set_show_image_in_report(image_client *client, ic_log *log, ic_bool show_image_in_report);
		ic_log log = { 0 };
		ic_impl_set_show_image_in_report(client, &log, show_image_in_report);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Decide whether to show screenshots of every step, or only failed steps in report.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_show_pass_image_in_report(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                             ic_bool show_pass_image_in_report /* when set to False will not show a screenshot of test steps that passed successfully (default=IC_TRUE) */)
	{
		IC_API void ic_impl_set_show_pass_image_in_report(image_client *client, ic_log *log, ic_bool show_pass_image_in_report);
		ic_log log = { 0 };
		ic_impl_set_show_pass_image_in_report(client, &log, show_pass_image_in_report);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			when set to False will not show reports steps.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_show_report(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                               ic_bool show_report /* when set to False will not show reports steps (default=IC_TRUE) */)
	{
		IC_API void ic_impl_set_show_report(image_client *client, ic_log *log, ic_bool show_report);
		ic_log log = { 0 };
		ic_impl_set_show_report(client, &log, show_report);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Load or unload sim card (to unlaod sim-card name is equal to null) to the currently used device
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_sim_card(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                            const char* sim_card_name /* the sim-card name, null to unload sim card form the device (default="") */)
	{
		IC_API void ic_impl_set_sim_card(image_client *client, ic_log *log, const char* sim_card_name);
		ic_log log = { 0 };
		ic_impl_set_sim_card(client, &log, sim_card_name);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_speed(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                         const char* speed /*  (default="") */)
	{
		IC_API void ic_impl_set_speed(image_client *client, ic_log *log, const char* speed);
		ic_log log = { 0 };
		ic_impl_set_speed(client, &log, speed);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Set the test as passed or failed
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SetTestStatus </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_test_status(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                               ic_bool status /* Status (default=IC_TRUE) */,
	                               const char* message /* Pass/Fail reason. (default="") */)
	{
		IC_API void ic_impl_set_test_status(image_client *client, ic_log *log, ic_bool status, const char* message);
		ic_log log = { 0 };
		ic_impl_set_test_status(client, &log, status, message);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Set web autoscroll
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_set_web_auto_scroll(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   ic_bool auto_scroll /* Set the autoscroll state (default=IC_TRUE) */)
	{
		IC_API void ic_impl_set_web_auto_scroll(image_client *client, ic_log *log, ic_bool auto_scroll);
		ic_log log = { 0 };
		ic_impl_set_web_auto_scroll(client, &log, auto_scroll);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Simulating Shake operation on the device.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Shake </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_shake(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_shake(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_shake(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 SimulateCapture
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SimulateCapture </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_simulate_capture(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                const char* picture_path /* picture's full path (default="") */)
	{
		IC_API void ic_impl_simulate_capture(image_client *client, ic_log *log, const char* picture_path);
		ic_log log = { 0 };
		ic_impl_simulate_capture(client, &log, picture_path);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Pause the script for a specified time.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Sleep </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_sleep(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                     int time /* The time to pause in MiliSec (default=1000) */)
	{
		IC_API void ic_impl_sleep(image_client *client, ic_log *log, int time);
		ic_log log = { 0 };
		ic_impl_sleep(client, &log, time);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Start play audio file
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_audio_play(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                const char* audio_file /* Set the audio file to start playing (default="") */)
	{
		IC_API void ic_impl_start_audio_play(image_client *client, ic_log *log, const char* audio_file);
		ic_log log = { 0 };
		ic_impl_start_audio_play(client, &log, audio_file);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Start record audio file
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_audio_recording(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                     const char* audio_file /* Set the audio file name (default="") */)
	{
		IC_API void ic_impl_start_audio_recording(image_client *client, ic_log *log, const char* audio_file);
		ic_log log = { 0 };
		ic_impl_start_audio_recording(client, &log, audio_file);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Start writing the current device's log to file
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_logging_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                    const char* path /* Absolute path to log file, or where the log file should be created (default="") */)
	{
		IC_API void ic_impl_start_logging_device(image_client *client, ic_log *log, const char* path);
		ic_log log = { 0 };
		ic_impl_start_logging_device(client, &log, path);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Clear monitoring collection data collected so far. If packageName is not empty, the application identified by this packageName will start being monitored.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/StartMonitor </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_monitor(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                             const char* package_name /* Package name (Android) or Bundle ID (iOS) of application (default="") */)
	{
		IC_API void ic_impl_start_monitor(image_client *client, ic_log *log, const char* package_name);
		ic_log log = { 0 };
		ic_impl_start_monitor(client, &log, package_name);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Start a group of multi-touch gesture steps.<br/>
	///	 Note: all steps will be performed only when executing the PerformMultiGestureStep command.<br/>
	///	 See online documentation for details
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/StartMultiGestureStep </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_multi_gesture(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* name /* Gesture name (default="") */)
	{
		IC_API void ic_impl_start_multi_gesture(image_client *client, ic_log *log, const char* name);
		ic_log log = { 0 };
		ic_impl_start_multi_gesture(client, &log, name);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Start grouping steps
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/StartStepsGroup </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_steps_group(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                 const char* caption /* The group's caption which will appear at the report (default="") */)
	{
		IC_API void ic_impl_start_steps_group(image_client *client, ic_log *log, const char* caption);
		ic_log log = { 0 };
		ic_impl_start_steps_group(client, &log, caption);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Start to measure transaction duration
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/StartTransaction </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_transaction(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                 const char* name /* Name of the transaction (default="") */)
	{
		IC_API void ic_impl_start_transaction(image_client *client, ic_log *log, const char* name);
		ic_log log = { 0 };
		ic_impl_start_transaction(client, &log, name);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Start the video recording.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/StartVideoRecord </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_start_video_record(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_start_video_record(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_start_video_record(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Stop audio playing
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_stop_audio_play(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_stop_audio_play(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_stop_audio_play(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Stop audio recording
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_stop_audio_recording(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_stop_audio_recording(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_stop_audio_recording(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Stop writing the current device's log to file
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_stop_logging_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_stop_logging_device(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_stop_logging_device(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Stop grouping steps
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/StopStepsGroup </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_stop_steps_group(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_stop_steps_group(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_stop_steps_group(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Stop the video recording.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/StopVideoRecord </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_stop_video_record(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API const char* ic_impl_stop_video_record(image_client *client, ic_log *log);
		ic_log log = { 0 };
		const char* result = ic_impl_stop_video_record(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Swipe the screen in a given direction
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/Swipe </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_swipe(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                     const char* direction /* Direction to swipe (default="") */,
	                     int offset /* Swipe offset (default=0) */,
	                     int time /* Swipe overall time (default=500) */)
	{
		IC_API void ic_impl_swipe(image_client *client, ic_log *log, const char* direction, int offset, int time);
		ic_log log = { 0 };
		ic_impl_swipe(client, &log, direction, offset, time);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Swipe a list to identify an element
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/SwipeWhileNotFound </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_swipe_while_not_found(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        const char* direction /* Direction to swipe (default="") */,
	                                        int offset /* Swipe offset (default=0) */,
	                                        int swipe_time /* Swipe operation time (default=2000) */,
	                                        const char* zone /* Select Zone (default="") */,
	                                        const char* elementtofind /* Select Element to Find (default="") */,
	                                        int elementtofindindex /* Element to Find Index (default=0) */,
	                                        int delay /* Time to wait before sending a command (in MiliSec) (default=1000) */,
	                                        int rounds /* Maximum swipe rounds (default=5) */,
	                                        ic_bool click /* If TRUE then click (default=IC_TRUE) */)
	{
		IC_API ic_bool ic_impl_swipe_while_not_found(image_client *client, ic_log *log, const char* direction, int offset, int swipe_time, const char* zone, const char* elementtofind, int elementtofindindex, int delay, int rounds, ic_bool click);
		ic_log log = { 0 };
		ic_bool result = ic_impl_swipe_while_not_found(client, &log, direction, offset, swipe_time, zone, elementtofind, elementtofindindex, delay, rounds, click);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Get the ID of the currently focused browser (Chrome / Safari) tab.
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_switch_to_browser_tab(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                     const char* tab_id /* Tab ID to switch to (default="") */)
	{
		IC_API void ic_impl_switch_to_browser_tab(image_client *client, ic_log *log, const char* tab_id);
		ic_log log = { 0 };
		ic_impl_switch_to_browser_tab(client, &log, tab_id);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Wait for the screen to be silent. Works on the graphical level of the screen.
	/// </summary>
	/// <returns> true if operation finished successfully </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/Sync </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_sync(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                       int silent_time /* The time for the screen to be silent in milliseconds (default=2000) */,
	                       int sensitivity /* Sensitivity from 0 - 100 (0 is maximum sensitivity) (default=0) */,
	                       int timeout /* Waiting timeout in milliseconds (default=10000) */)
	{
		IC_API ic_bool ic_impl_sync(image_client *client, ic_log *log, int silent_time, int sensitivity, int timeout);
		ic_log log = { 0 };
		ic_bool result = ic_impl_sync(client, &log, silent_time, sensitivity, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Wait for all the UI elements on the page to appear. Works on the dump level - checks for changes in the UI dump.
	/// </summary>
	/// <returns> true if operation finished successfully </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/SyncElements </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_sync_elements(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                int silent_time /* The period of time for the UI elements to be static in milliseconds. (default=2000) */,
	                                int timeout /* Waiting timeout in milliseconds (default=10000) */)
	{
		IC_API ic_bool ic_impl_sync_elements(image_client *client, ic_log *log, int silent_time, int timeout);
		ic_log log = { 0 };
		ic_bool result = ic_impl_sync_elements(client, &log, silent_time, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Filter the text recognition text color for the next command.<br/>
	///	 			This setting will be applied on the next command only.<br/>
	///	 			It's only supported when you use text recognition (OCR).
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/TextFilter </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_text_filter(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                           const char* color /* text color in RGB hex decimal string (0xFFFFFF for white). (default="") */,
	                           int sensitivity /* Sensitivity from 0 - 100 (0 is no sensitivity) (default=15) */)
	{
		IC_API void ic_impl_text_filter(image_client *client, ic_log *log, const char* color, int sensitivity);
		ic_log log = { 0 };
		ic_impl_text_filter(client, &log, color, sensitivity);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Hold Touch down on element, Release with TouchUp command.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/TouchDown </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_touch_down(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                          const char* zone /* Select Zone (default="") */,
	                          const char* element /* Select Element (default="") */,
	                          int index /* Element Order (default=0) */)
	{
		IC_API void ic_impl_touch_down(image_client *client, ic_log *log, const char* zone, const char* element, int index);
		ic_log log = { 0 };
		ic_impl_touch_down(client, &log, zone, element, index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Hold Touch down at X,Y coordinates related to the device screen, Release with TouchUp command.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/TouchDownCoordinate </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_touch_down_coordinate(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                     int x /* Horizontal coordinate (default=0) */,
	                                     int y /* Vertical coordinate (default=0) */)
	{
		IC_API void ic_impl_touch_down_coordinate(image_client *client, ic_log *log, int x, int y);
		ic_log log = { 0 };
		ic_impl_touch_down_coordinate(client, &log, x, y);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Move from last Touched down element/coordinate to specified element location.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/TouchMove </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_touch_move(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                          const char* zone /* Select Zone (default="") */,
	                          const char* element /* Select Element (default="") */,
	                          int index /* Element Order (default=0) */)
	{
		IC_API void ic_impl_touch_move(image_client *client, ic_log *log, const char* zone, const char* element, int index);
		ic_log log = { 0 };
		ic_impl_touch_move(client, &log, zone, element, index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Move from last Touched down element/coordinate to specified coordinate location.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/TouchMoveCoordinate </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_touch_move_coordinate(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                     int x /* Horizontal coordinate (default=0) */,
	                                     int y /* Vertical coordinate (default=0) */)
	{
		IC_API void ic_impl_touch_move_coordinate(image_client *client, ic_log *log, int x, int y);
		ic_log log = { 0 };
		ic_impl_touch_move_coordinate(client, &log, x, y);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Touch Up from last coordinate touched down or moved to.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/TouchUp </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_touch_up(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */)
	{
		IC_API void ic_impl_touch_up(image_client *client, ic_log *log);
		ic_log log = { 0 };
		ic_impl_touch_up(client, &log);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Uninstall the application
	/// </summary>
	/// <returns> uninstallation success </returns>
	/// <documentation> https://docs.experitest.com/display/public/SA/Uninstall </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_uninstall(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                            const char* application /* The application name (default="") */)
	{
		IC_API ic_bool ic_impl_uninstall(image_client *client, ic_log *log, const char* application);
		ic_log log = { 0 };
		ic_bool result = ic_impl_uninstall(client, &log, application);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 			Check if an element is found in a specified zone.<br/>
	///	 			An Exception (or Assertion) will be thrown if the element is not found.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/VerifyElementFound </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_verify_element_found(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                    const char* zone /* Select Zone (default="") */,
	                                    const char* element /* Select Element (default="") */,
	                                    int index /* Element Order (=the number of times the element appears more and above the first time) (default=0) */)
	{
		IC_API void ic_impl_verify_element_found(image_client *client, ic_log *log, const char* zone, const char* element, int index);
		ic_log log = { 0 };
		ic_impl_verify_element_found(client, &log, zone, element, index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Check if an element is found in a specified zone.<br/>
	///	 			An Exception (or Assertion) will be thrown if the element is found.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/VerifyElementNotFound </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_verify_element_not_found(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                        const char* zone /* Select Zone (default="") */,
	                                        const char* element /* Select Element (default="") */,
	                                        int index /* Element Order (=the number of times the element appears more and above the first time) (default=0) */)
	{
		IC_API void ic_impl_verify_element_not_found(image_client *client, ic_log *log, const char* zone, const char* element, int index);
		ic_log log = { 0 };
		ic_impl_verify_element_not_found(client, &log, zone, element, index);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 			Verify that an element("elementToFind") is found near to another element("elementSearch").<br/>
	///	 			The direction can be UP, DOWN, LEFT and RIGHT.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/VerifyIn </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_verify_in(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                         const char* zone /* Select Zone (default="") */,
	                         const char* search_element /* Search Element (default="") */,
	                         int index /* Element index (default=0) */,
	                         const char* direction /* Direction to analyze (default="") */,
	                         const char* element_find_zone /* Find Element Zone (default="") */,
	                         const char* element_to_find /* Element to Find (default="") */,
	                         int width /* Width of the search (0 indicate until the end/start of the window) (default=0) */,
	                         int height /* Height of the search (0 indicate until the end/start of the window) (default=0) */)
	{
		IC_API void ic_impl_verify_in(image_client *client, ic_log *log, const char* zone, const char* search_element, int index, const char* direction, const char* element_find_zone, const char* element_to_find, int width, int height);
		ic_log log = { 0 };
		ic_impl_verify_in(client, &log, zone, search_element, index, direction, element_find_zone, element_to_find, width, height);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Wait for audio file play to end
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static void ic_wait_for_audio_play_end(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                       int timeout /* Set the timeout */)
	{
		IC_API void ic_impl_wait_for_audio_play_end(image_client *client, ic_log *log, int timeout);
		ic_log log = { 0 };
		ic_impl_wait_for_audio_play_end(client, &log, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
	}


	/*
	/// <summary>
	///	 Wait for a device. Release command should be used to enable other tests to use this device again.
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/WaitForDevice </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static const char* ic_wait_for_device(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                      const char* device /* Search device query, for example: @os='android' and @version='4.4.2' (default="") */,
	                                      int timeout /* Timeout in milliseconds (default=300000) */)
	{
		IC_API const char* ic_impl_wait_for_device(image_client *client, ic_log *log, const char* device, int timeout);
		ic_log log = { 0 };
		const char* result = ic_impl_wait_for_device(client, &log, device, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Wait for an element to appear in a specified zone
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/WaitForElement </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_wait_for_element(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                   const char* zone /* Select Zone (default="") */,
	                                   const char* element /* Select Element to Wait For (default="") */,
	                                   int index /* Element Order (default=0) */,
	                                   int timeout /* Waiting Timeout in MiliSec (default=10000) */)
	{
		IC_API ic_bool ic_impl_wait_for_element(image_client *client, ic_log *log, const char* zone, const char* element, int index, int timeout);
		ic_log log = { 0 };
		ic_bool result = ic_impl_wait_for_element(client, &log, zone, element, index, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Wait for an element to vanish
	/// </summary>
	/// <documentation> https://docs.experitest.com/display/public/SA/WaitForElementToVanish </documentation>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_wait_for_element_to_vanish(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                             const char* zone /* Select Zone (default="") */,
	                                             const char* element /* Select Element to Wait For (default="") */,
	                                             int index /* Element Order (default=0) */,
	                                             int timeout /* Waiting Timeout in MiliSec (default=10000) */)
	{
		IC_API ic_bool ic_impl_wait_for_element_to_vanish(image_client *client, ic_log *log, const char* zone, const char* element, int index, int timeout);
		ic_log log = { 0 };
		ic_bool result = ic_impl_wait_for_element_to_vanish(client, &log, zone, element, index, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}


	/*
	/// <summary>
	///	 Wait for a window
	/// </summary>
	/// <remarks> On error or internal exception, you can look up for the error description under client.latest_result.details.internal_exception.message </remarks>
	*/
	static ic_bool ic_wait_for_window(image_client *client /* A pointer to an 'image_client' handler (may not be NULL!) */,
	                                  const char* name /* Window name (default="") */,
	                                  int timeout /* Waiting Timeout in MiliSec (default=10000) */)
	{
		IC_API ic_bool ic_impl_wait_for_window(image_client *client, ic_log *log, const char* name, int timeout);
		ic_log log = { 0 };
		ic_bool result = ic_impl_wait_for_window(client, &log, name, timeout);
		ic_output_log(&log);
		ic_process_exception(client);
		return result;
	}




	/*
	/// <summary>
	/// Sets whether to print debug messages related to this client.
	/// </summary>
	*/
	IC_API void ic_set_client_debug_status(image_client *client, /* A pointer to the 'image_client' handler to be released (may not be NULL!) */
		                                   ic_bool debug_status  /* Output debug messages iff true*/);



	static void ic_output_log(const ic_log *log) {
		int i;
		for (i = 0; i < log->messages.length && i < IC_MAX_MESSAGE_NUMBER; ++i) {
			if (log->arr_levels[i] <= 25)
				lr_error_message("[ERROR]: %s", log->messages.array[i]);
			else
				lr_message("[%s]: %s", log->arr_levels[i] <= 50 ? "INFO" : "DEBUG", log->messages.array[i]);
		}
	}


	static void ic_process_exception(image_client *client) {
		void(*orig_callback)(image_client *);
		if (client != NULL && client->latest_result.status != IC_DONE && (orig_callback = client->exception_callback) != 0) {
			client->exception_callback = 0;
			(*orig_callback)(client);
			client->exception_callback = orig_callback;
		}
	}




#ifdef __cplusplus
	}
#endif

#endif /* IC_API_DOT_H__ */
