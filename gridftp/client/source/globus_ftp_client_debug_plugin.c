#ifndef GLOBUS_DONT_DOCUMENT_INTERNAL
/**
 * @file globus_ftp_client_debug_plugin.c GridFTP Debugging Plugin Implementation
 *
 * $RCSfile$
 * $Revision$
 * $Date$
 */
#endif

#include "globus_ftp_client_debug_plugin.h"

#include <stdio.h>
#include <string.h>

#define GLOBUS_L_FTP_CLIENT_DEBUG_PLUGIN_NAME "globus_ftp_client_debug_plugin"

#define GLOBUS_L_FTP_CLIENT_DEBUG_PLUGIN_RETURN(plugin) \
    if(plugin == GLOBUS_NULL) \
    {\
	return globus_error_put(globus_error_construct_string(\
		GLOBUS_FTP_CLIENT_MODULE,\
		GLOBUS_NULL,\
		"[%s] NULL plugin at %s\n",\
		GLOBUS_FTP_CLIENT_MODULE->module_name,\
		myname));\
    }
#define GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(d, func) \
    result = globus_ftp_client_plugin_set_##func##_func(d, globus_l_ftp_client_debug_plugin_##func); \
    if(result != GLOBUS_SUCCESS) goto result_exit;

typedef struct
{
    FILE *					stream;
    char *					text;
}
globus_l_ftp_client_debug_plugin_t;

static
globus_ftp_client_plugin_t *
globus_l_ftp_client_debug_plugin_copy(
    globus_ftp_client_plugin_t *		plugin_template,
    void *					plugin_specific);

static
void
globus_l_ftp_client_debug_plugin_destroy(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific);

static
void
globus_l_ftp_client_debug_plugin_connect(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url);

static
void
globus_l_ftp_client_debug_plugin_authenticate(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    const globus_ftp_control_auth_info_t *	auth_info);

static
void
globus_l_ftp_client_debug_plugin_delete(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_mkdir(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_rmdir(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_list(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_verbose_list(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_move(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				source_url,
    const char *				dest_url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_get(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_put(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_third_party_transfer(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				source_url,
    const globus_ftp_client_operationattr_t *	source_attr,
    const char *				dest_url,
    const globus_ftp_client_operationattr_t *	dest_attr,
    globus_bool_t 				restart);

static
void
globus_l_ftp_client_debug_plugin_abort(
    globus_ftp_client_plugin_t *		plugin,
    void * 					plugin_specific,
    globus_ftp_client_handle_t *		handle);

static
void
globus_l_ftp_client_debug_plugin_read(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_byte_t *			buffer,
    globus_size_t 				buffer_length);

static
void
globus_l_ftp_client_debug_plugin_write(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_byte_t *			buffer,
    globus_size_t 				buffer_length,
    globus_off_t				offset,
    globus_bool_t				eof);

static
void
globus_l_ftp_client_debug_plugin_data(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    globus_object_t *				error,
    const globus_byte_t *			buffer,
    globus_size_t 				length,
    globus_off_t				offset,
    globus_bool_t				eof);

static
void
globus_l_ftp_client_debug_plugin_command(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    const char *				command_name);

static
void
globus_l_ftp_client_debug_plugin_response(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    globus_object_t *				error,
    const globus_ftp_control_response_t *	ftp_response);

static
void
globus_l_ftp_client_debug_plugin_fault(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    globus_object_t *				error);

static
void
globus_l_ftp_client_debug_plugin_complete(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle);

static
globus_ftp_client_plugin_t *
globus_l_ftp_client_debug_plugin_copy(
    globus_ftp_client_plugin_t *		plugin_template,
    void *					plugin_specific)
{
    globus_ftp_client_plugin_t *		newguy;
    globus_l_ftp_client_debug_plugin_t *	d;
    globus_result_t				result;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    newguy = globus_libc_malloc(sizeof(globus_ftp_client_plugin_t));
    if(newguy == GLOBUS_NULL)
    {
	goto error_exit;
    }
    result = globus_ftp_client_debug_plugin_init(newguy);
    if(result != GLOBUS_SUCCESS)
    {
	goto free_exit;
    }
    result = globus_ftp_client_debug_plugin_set_stream(newguy,
					               d->stream);
    if(result != GLOBUS_SUCCESS)
    {
	goto destroy_exit;
    }
    result = globus_ftp_client_debug_plugin_set_text(newguy,
	                                             d->text);
    if(result != GLOBUS_SUCCESS)
    {
	goto destroy_exit;
    }
    return newguy;

destroy_exit:
    globus_ftp_client_debug_plugin_destroy(newguy);
free_exit:
    globus_libc_free(newguy);
error_exit:
    return GLOBUS_NULL;
}
/* globus_l_ftp_client_debug_plugin_copy() */

static
void
globus_l_ftp_client_debug_plugin_destroy(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific)
{
    globus_ftp_client_debug_plugin_destroy(plugin);
    globus_libc_free(plugin);
}
/* globus_l_ftp_client_debug_plugin_destroy() */

static
void
globus_l_ftp_client_debug_plugin_connect(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url)
{
    globus_l_ftp_client_debug_plugin_t *	d;
    char url_port[10];

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    if(url->port)
    {
	sprintf(url_port, "%u", url->port);
    }
    else
    {
	url_port[0] = '\0';
    }

    fprintf(d->stream, "%s%sconnecting to %s://%s%s%s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url->scheme,
	    url->host,
	    url->port ? ":" : "",
	    url->port ? url_port : "");
}
/* globus_l_ftp_client_debug_plugin_connect() */

static
void
globus_l_ftp_client_debug_plugin_authenticate(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    const globus_ftp_control_auth_info_t *	auth_info)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sauthenticating with %s://%s%s%s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url->host);
}
/* globus_l_ftp_client_debug_plugin_authenticate() */

static
void
globus_l_ftp_client_debug_plugin_delete(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to delete %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url);
}
/* globus_l_ftp_client_debug_plugin_delete() */

static
void
globus_l_ftp_client_debug_plugin_mkdir(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to mkdir %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url);
}
/* globus_l_ftp_client_debug_plugin_mkdir() */

static
void
globus_l_ftp_client_debug_plugin_rmdir(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to rmdir %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url);
}
/* globus_l_ftp_client_debug_plugin_rmdir() */

static
void
globus_l_ftp_client_debug_plugin_list(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to list %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url);
}
/* globus_l_ftp_client_debug_plugin_list() */

static
void
globus_l_ftp_client_debug_plugin_verbose_list(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to verbose list %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url);
}
/* globus_l_ftp_client_debug_plugin_vlist() */

static
void
globus_l_ftp_client_debug_plugin_move(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				source_url,
    const char *				dest_url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to move %s to %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    source_url,
	    dest_url);
}
/* globus_l_ftp_client_debug_plugin_move() */

static
void
globus_l_ftp_client_debug_plugin_get(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to get %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url);
}
/* globus_l_ftp_client_debug_plugin_get() */

static
void
globus_l_ftp_client_debug_plugin_put(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				url,
    const globus_ftp_client_operationattr_t *	attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to put %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    url);
}
/* globus_l_ftp_client_debug_plugin_put() */

static
void
globus_l_ftp_client_debug_plugin_third_party_transfer(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const char *				source_url,
    const globus_ftp_client_operationattr_t *	source_attr,
    const char *				dest_url,
    const globus_ftp_client_operationattr_t *	dest_attr,
    globus_bool_t 				restart)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sstarting to transfer %s to %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    source_url,
	    dest_url);
}
/* globus_l_ftp_client_debug_plugin_third_party_transfer() */

static
void
globus_l_ftp_client_debug_plugin_abort(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%saborting current operation\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "");
}
/* globus_l_ftp_client_debug_plugin_abort() */

static
void
globus_l_ftp_client_debug_plugin_read(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_byte_t *			buffer,
    globus_size_t 				buffer_length)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sreading into data buffer %p, maximum length %ld\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    buffer,
	    buffer_length);
}
/* globus_l_ftp_client_debug_plugin_read() */

static
void
globus_l_ftp_client_debug_plugin_write(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_byte_t *			buffer,
    globus_size_t 				buffer_length,
    globus_off_t				offset,
    globus_bool_t				eof)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%swriting buffer %p, length %ld, "
	           "offset=%"GLOBUS_OFF_T_FORMAT", eof=%s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    buffer,
	    buffer_length,
	    offset,
	    eof ? "true" : "false");
}
/* globus_l_ftp_client_debug_plugin_write() */

static
void
globus_l_ftp_client_debug_plugin_data(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    globus_object_t *				error,
    const globus_byte_t *			buffer,
    globus_size_t 				length,
    globus_off_t				offset,
    globus_bool_t				eof)
{
    globus_l_ftp_client_debug_plugin_t *	d;
    char * error_str;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(error)
    {
        error_str = globus_object_printable_to_string(error);
    }
    else
    {
	error_str = GLOBUS_NULL;
    }

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%sdata callback, %serror%s%s, buffer %p, length %ld, "
	           "offset=%"GLOBUS_OFF_T_FORMAT", eof=%s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    error_str ? "" : "no ",
	    error_str ? " " : "",
	    error_str ? error_str : "",
	    buffer,
	    length,
	    offset,
	    eof ? "true" : "false");
    if(error_str)
    {
	globus_libc_free(error_str);
    }
}
/* globus_l_ftp_client_debug_plugin_data() */

static
void
globus_l_ftp_client_debug_plugin_command(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    const char *				command_name)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%ssending command %s\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "",
	    command_name);
}
/* globus_l_ftp_client_debug_plugin_command() */

static
void
globus_l_ftp_client_debug_plugin_response(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    globus_object_t *				error,
    const globus_ftp_control_response_t *	ftp_response)
{
    globus_l_ftp_client_debug_plugin_t *	d;
    char * error_str;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    if(!error)
    {
	fprintf(d->stream, "%s%sresponse from %s: %s\n",
		d->text ? d->text : "",
		d->text ? ": " : "",
		url->host,
		ftp_response->response_buffer);
    }
    else
    {
	error_str = globus_object_printable_to_string(error);

	fprintf(d->stream, "%s%serror reading response from %s: %s\n",
		d->text ? d->text : "",
		d->text ? ": " : "",
		url->host,
		error_str);

	globus_libc_free(error_str);
    }
}
/* globus_l_ftp_client_debug_plugin_response() */

static
void
globus_l_ftp_client_debug_plugin_fault(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle,
    const globus_url_t *			url,
    globus_object_t *				error)
{
    globus_l_ftp_client_debug_plugin_t *	d;
    char * error_str;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    if(!error)
    {
	fprintf(d->stream, "%s%sfault on connection to %s\n",
		d->text ? d->text : "",
		d->text ? ": " : "",
		url->host);
    }
    else
    {
	error_str = globus_object_printable_to_string(error);

	fprintf(d->stream, "%s%sfault on connection to %s: %s\n",
		d->text ? d->text : "",
		d->text ? ": " : "",
		url->host,
		error_str);

	globus_libc_free(error_str);
    }
}
/* globus_l_ftp_client_debug_plugin_fault() */

static
void
globus_l_ftp_client_debug_plugin_complete(
    globus_ftp_client_plugin_t *		plugin,
    void *					plugin_specific,
    globus_ftp_client_handle_t *		handle)
{
    globus_l_ftp_client_debug_plugin_t *	d;

    d = (globus_l_ftp_client_debug_plugin_t *) plugin_specific;

    if(!d->stream)
    {
	return;
    }

    fprintf(d->stream, "%s%soperation complete\n",
	    d->text ? d->text : "",
	    d->text ? ": " : "");
}
/* globus_l_ftp_client_debug_plugin_complete() */

/**
 * Initialize an instance of the GridFTP debugging plugin
 * @ingroup globus_ftp_client_debug_plugin
 *
 * This function will initialize the debugging plugin-specific instance data
 * for this plugin, and will make the plugin usable for ftp
 * client handle attribute and handle creation.
 *
 * @param plugin
 *        A pointer to an uninitialized plugin. The plugin will be
 *        configured as a debugging plugin, with the default of sending
 *        debugging messages to stderr.
 *
 * @return This function returns an error if
 * - plugin is null
 *
 * @see globus_ftp_client_debug_plugin_destroy(),
 *      globus_ftp_client_handleattr_add_plugin(),
 *      globus_ftp_client_handleattr_remove_plugin(),
 *      globus_ftp_client_handle_init()
 */
globus_result_t
globus_ftp_client_debug_plugin_init(
    globus_ftp_client_plugin_t *		plugin)
{
    globus_l_ftp_client_debug_plugin_t *	d;
    globus_object_t *				err;
    globus_result_t				result;
    static char * myname = "globus_ftp_client_debug_plugin_init";

    if(plugin == GLOBUS_NULL)
    {
	return globus_error_put(globus_error_construct_string(
		GLOBUS_FTP_CLIENT_MODULE,
		GLOBUS_NULL,
		"[%s] NULL plugin at %s\n",
		GLOBUS_FTP_CLIENT_MODULE->module_name,
		myname));
    }

    d =
	globus_libc_malloc(sizeof(globus_l_ftp_client_debug_plugin_t));

    if(! d)
    {
	return globus_error_put(globus_error_construct_string(
		                GLOBUS_FTP_CLIENT_MODULE,
				GLOBUS_NULL,
				"[%s] Out of memory at %s\n",
				 GLOBUS_FTP_CLIENT_MODULE->module_name,
				 myname));
    }

    d->stream = stderr;
    d->text = GLOBUS_NULL;

    result = globus_ftp_client_plugin_init(plugin,
				  GLOBUS_L_FTP_CLIENT_DEBUG_PLUGIN_NAME,
				  GLOBUS_FTP_CLIENT_CMD_MASK_ALL,
				  d);
    if(result != GLOBUS_SUCCESS)
    {
	globus_libc_free(d);

	return result;
    }

    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, copy);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, destroy);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, delete);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, mkdir);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, rmdir);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, move);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, verbose_list);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, list);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, get);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, put);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, third_party_transfer);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, abort);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, connect);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, authenticate);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, read);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, data);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, command);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, response);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, fault);
    GLOBUS_FTP_CLIENT_DEBUG_PLUGIN_SET_FUNC(plugin, complete);

    return GLOBUS_SUCCESS;

result_exit:
    err = globus_error_get(result);
    globus_ftp_client_plugin_destroy(plugin);
    return globus_error_put(result);
}
/* globus_ftp_client_debug_plugin_init() */

/**
 * Set the debugging plugin's output stream.
 * @ingroup globus_ftp_client_debug_plugin
 *
 * This function sets the FILE stream to be used by the GridFTP debugging
 * plugin. This stream must not be closed while a GridFTP handle has a
 * reference to this plugin.
 *
 * @param plugin
 *        A pointer to a GridFTP debugging plugin, previously initialized by
 *        calling globus_ftp_client_debug_plugin_init()
 * @param stream
 *        A stdio FILE stream pointer to which debugging messages should be
 *        sent.
 *
 * @return This function returns an error if
 * - plugin is NULL
 * - stream is NULL
 * - plugin is not a debugging plugin
 */
globus_result_t
globus_ftp_client_debug_plugin_set_stream(
    globus_ftp_client_plugin_t *		plugin,
    FILE *					stream)
{
    globus_l_ftp_client_debug_plugin_t *	d;
    globus_result_t				result;
    static char * myname = "globus_ftp_client_debug_plugin_set_stream";

    GLOBUS_L_FTP_CLIENT_DEBUG_PLUGIN_RETURN(plugin);

    if(stream == GLOBUS_NULL)
    {
	return globus_error_put(globus_error_construct_string(\
		GLOBUS_FTP_CLIENT_MODULE,\
		GLOBUS_NULL,\
		"[%s] Invalid stream at %s\n",\
		GLOBUS_FTP_CLIENT_MODULE->module_name,\
		myname));\
    }

    result = globus_ftp_client_plugin_get_plugin_specific(plugin,
	                                                  (void **) &d);
    if(result != GLOBUS_SUCCESS)
    {
	return result;
    }
    d->stream = stream;

    return GLOBUS_SUCCESS;
}
/* globus_ftp_client_debug_plugin_set_stream() */

/**
 * Set the debugging plugin's text.
 * @ingroup globus_ftp_client_debug_plugin
 *
 * This function sets the text to be used to start all messages printed to
 * the plugins FILE stream. This may be NULL (the default), if no
 * application-specific message is needed.
 *
 * Messages from teh debugging plugin will be prefixed by the text string
 * followed by a colon, if the text string is non-NULL.
 *
 * @param plugin
 *        A pointer to a GridFTP debugging plugin, previously initialized by
 *        calling globus_ftp_client_debug_plugin_init()
 * @param text
 *        An arbitrary string to be prepended to the debugging plugin's
 *        messages.
 *
 * @return This function returns an error if
 * - plugin is NULL
 * - stream is NULL
 * - plugin is not a debugging plugin
 */
globus_result_t
globus_ftp_client_debug_plugin_set_text(
    globus_ftp_client_plugin_t *		plugin,
    const char *				text)
{
    globus_l_ftp_client_debug_plugin_t *	d;
    globus_result_t				result;
    char *					tmp;
    static char * myname = "globus_ftp_client_debug_plugin_set_text";

    GLOBUS_L_FTP_CLIENT_DEBUG_PLUGIN_RETURN(plugin);

    result = globus_ftp_client_plugin_get_plugin_specific(plugin,
	                                                  (void **) &d);
    if(result != GLOBUS_SUCCESS)
    {
	return result;
    }

    tmp = d->text;

    if(text)
    {
	d->text = globus_libc_strdup(text);
	if(d->text == GLOBUS_NULL)
	{
	    d->text = tmp;

	    return globus_error_put(globus_error_construct_string(
			GLOBUS_FTP_CLIENT_MODULE,
			GLOBUS_NULL,
			"[%s] Out of memory at %s\n",
			 GLOBUS_FTP_CLIENT_MODULE->module_name,
			 myname));
	}
    }
    else
    {
	d->text = GLOBUS_NULL;
    }
    if(tmp)
    {
	globus_libc_free(tmp);
    }

    return GLOBUS_SUCCESS;
}
/* globus_ftp_client_debug_plugin_set_text() */

/**
 * Destroy an instance of the GridFTP debugging plugin
 * @ingroup globus_ftp_client_debug_plugin
 *
 * This function will free all debugging plugin-specific instance data
 * from this plugin, and will make the plugin unusable for further ftp
 * handle creation.
 *
 * Existing FTP client handles and handle attributes will not be affected by
 * destroying a plugin associated with them, as a local copy of the plugin
 * is made upon handle initialization.
 *
 * @param plugin
 *        A pointer to a GridFTP debugging plugin, previously initialized by
 *        calling globus_ftp_client_debug_plugin_init()
 *
 * @return This function returns an error if
 * - plugin is null
 * - plugin is not a debugging plugin
 *
 * @see globus_ftp_client_debug_plugin_init(),
 *      globus_ftp_client_handleattr_add_plugin(),
 *      globus_ftp_client_handleattr_remove_plugin(),
 *      globus_ftp_client_handle_init()
 */
globus_result_t
globus_ftp_client_debug_plugin_destroy(
    globus_ftp_client_plugin_t *		plugin)
{
    globus_l_ftp_client_debug_plugin_t * d;
    globus_result_t result;
    static char * myname = "globus_ftp_client_debug_plugin_destroy";

    GLOBUS_L_FTP_CLIENT_DEBUG_PLUGIN_RETURN(plugin);

    result = globus_ftp_client_plugin_get_plugin_specific(plugin,
	                                                  (void **) &d);
    if(result != GLOBUS_SUCCESS)
    {
	return result;
    }

    if(d->text)
    {
	globus_libc_free(d->text);
    }

    return globus_ftp_client_plugin_destroy(plugin);
}
/* globus_ftp_client_debug_plugin_destroy() */
