#include <stdio.h>
#include <stdlib.h>
#include <evhttp.h>
#include <event.h>
#include <string.h>
#include <iostream>
#include "event2/http.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_compat.h"
#include "event2/http_struct.h"
#include "event2/http_compat.h"
#include "event2/util.h"
#include "event2/listener.h"

#define BUF_MAX 1024*16

//����post��������
void get_post_message(char *buf, struct evhttp_request *req)
{
	size_t post_size = 0;

	post_size = evbuffer_get_length(req->input_buffer);//��ȡ���ݳ���
	printf("====line:%d,post len:%d\n", __LINE__, post_size);
	if (post_size <= 0)
	{
		printf("====line:%d,post msg is empty!\n", __LINE__);
		return;
	}
	else
	{
		size_t copy_len = post_size > BUF_MAX ? BUF_MAX : post_size;
		printf("====line:%d,post len:%d, copy_len:%d\n", __LINE__, post_size, copy_len);
		memcpy(buf, evbuffer_pullup(req->input_buffer, -1), copy_len);
		buf[post_size] = '\0';
		printf("====line:%d,post msg:%s\n", __LINE__, buf);
	}
}

//����httpͷ����Ҫ����get����ʱ����uri���������
char *find_http_header(struct evhttp_request *req, struct evkeyvalq *params, const char *query_char)
{
	if (req == NULL || params == NULL || query_char == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "input params is null.");
		return NULL;
	}

	struct evhttp_uri *decoded = NULL;
	char *query = NULL;
	char *query_result = NULL;
	const char *path;
	const char *uri = evhttp_request_get_uri(req);//��ȡ����uri

	if (uri == NULL)
	{
		printf("====line:%d,evhttp_request_get_uri return null\n", __LINE__);
		return NULL;
	}
	else
	{
		printf("====line:%d,Got a GET request for <%s>\n", __LINE__, uri);
	}

	//����uri
	decoded = evhttp_uri_parse(uri);
	if (!decoded)
	{
		printf("====line:%d,It's not a good URI. Sending BADREQUEST\n", __LINE__);
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return NULL;
	}

	//��ȡuri�е�path����
	path = evhttp_uri_get_path(decoded);
	if (path == NULL)
	{
		path = "/";
	}
	else
	{
		printf("====line:%d,path is:%s\n", __LINE__, path);
	}

	//��ȡuri�еĲ�������
	query = (char*)evhttp_uri_get_query(decoded);
	if (query == NULL)
	{
		printf("====line:%d,evhttp_uri_get_query return null\n", __LINE__);
		return NULL;
	}

	//��ѯָ��������ֵ
	evhttp_parse_query_str(query, params);
	query_result = (char*)evhttp_find_header(params, query_char);

	return query_result;
}



void get_autopatch(std::string path, std::string& data) {
	if (path.empty()) {
		printf("====line:%d,%s\n", __LINE__, "input path is null.");
		return;
	}

	FILE*fp = fopen(path.c_str(), "r");
	if (NULL == fp)
	{
		perror("fopen");
		printf("====line:%d,%s\n", __LINE__, "fopen error");
		return;
	}

	int nFileLen = 0;
	fseek(fp, 0, SEEK_END); //��λ���ļ�ĩ

	if ((nFileLen = ftell(fp)) < 1)//�ļ�����

	{

		fclose(fp);
		printf("====line:%d,%s\n", __LINE__, "ftell error");
		return ;

	}
	fseek(fp, 0, SEEK_SET); //��λ���ļ�ĩ


	char* fdata = (char *)malloc(sizeof(char)*(nFileLen + 1));
	memset(fdata,0, nFileLen + 1);

	if (NULL == fdata)

	{

		fclose(fp);
		printf("====line:%d,%s\n", __LINE__, "malloc error");
		return ;

	}

	fread(fdata, nFileLen, 1, fp);

	data = fdata;

	fclose(fp);//fopen֮��ǵ�fclose
	return ;

}


//����get����
void http_handler_testget_msg(struct evhttp_request *req, void *arg)
{
	if (req == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "input param req is null.");
		return;
	}

	char *sign = NULL;
	char *data = NULL;
	struct evkeyvalq sign_params = { 0 };
	sign = find_http_header(req, &sign_params, "sign");//��ȡget����uri�е�sign����
	if (sign == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "request uri no param sign.");
	}
	else
	{
		printf("====line:%d,get request param: sign=[%s]\n", __LINE__, sign);
	}

	data = find_http_header(req, &sign_params, "data");//��ȡget����uri�е�data����
	if (data == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "request uri no param data.");
	}
	else
	{
		printf("====line:%d,get request param: data=[%s]\n", __LINE__, data);
	}
	printf("\n");

	//����Ӧ
	struct evbuffer *retbuff = NULL;
	retbuff = evbuffer_new();
	if (retbuff == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
		return;
	}

	std::string fdata;
	get_autopatch("./AutoPatch.ini", fdata);



	evbuffer_add_printf(retbuff, "[%s]",fdata.c_str());
	evhttp_send_reply(req, HTTP_OK, "Client", retbuff);
	evbuffer_free(retbuff);
}

//����post����
void http_handler_testpost_msg(struct evhttp_request *req, void *arg)
{
	if (req == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "input param req is null.");
		return;
	}

	char buf[BUF_MAX] = { 0 };
	get_post_message(buf, req);//��ȡ�������ݣ�һ����json��ʽ������
	if (buf == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "get_post_message return null.");
		return;
	}
	else
	{
		//����ʹ��json�������Ҫ������
		printf("====line:%d,request data:%s", __LINE__, buf);
	}

	//����Ӧ
	struct evbuffer *retbuff = NULL;
	retbuff = evbuffer_new();
	if (retbuff == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
		return;
	}
	evbuffer_add_printf(retbuff, "Receive post request,Thamks for the request!");
	evhttp_send_reply(req, HTTP_OK, "Client", retbuff);
	evbuffer_free(retbuff);
}


void http_handler_testme_msg(struct evhttp_request *req, void *arg)
{
	if (req == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "input param req is null.");
		return;
	}

	char buf[BUF_MAX] = { 0 };
	get_post_message(buf, req);//��ȡ�������ݣ�һ����json��ʽ������
	if (buf == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "http_handler_testme_msg return null.");
		return;
	}
	else
	{
		//����ʹ��json�������Ҫ������
		printf("====line:%d,request data:%s", __LINE__, buf);
	}

	//����Ӧ
	struct evbuffer *retbuff = NULL;
	retbuff = evbuffer_new();
	if (retbuff == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
		return;
	}
	evbuffer_add_printf(retbuff, "Receive post request,Thamks for the request!");
	evhttp_send_reply(req, HTTP_NOTFOUND, "Client", retbuff);
	evbuffer_free(retbuff);
}

int main()
{
	struct evhttp *http_server = NULL;
	short http_port = 8081;
	char *http_addr = "0.0.0.0";

	//��ʼ��
	event_init();
	//����http�����
	http_server = evhttp_start(http_addr, http_port);
	if (http_server == NULL)
	{
		printf("====line:%d,%s\n", __LINE__, "http server start failed.");
		return -1;
	}

	//��������ʱʱ��(s)
	evhttp_set_timeout(http_server, 5);
	//�����¼���������evhttp_set_cb���ÿһ���¼�(����)ע��һ����������
	//������evhttp_set_gencb�������Ƕ�������������һ��ͳһ�Ĵ�����
	evhttp_set_cb(http_server, "/me/testpost", http_handler_testpost_msg, NULL);
	evhttp_set_cb(http_server, "/me/testget", http_handler_testget_msg, NULL);
	evhttp_set_cb(http_server, "/me/msg", http_handler_testme_msg, NULL);

	//ѭ������
	event_dispatch();
	//ʵ���ϲ����ͷţ����벻�����е���һ��
	evhttp_free(http_server);

	return 0;
}
