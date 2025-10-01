#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <pthread.h>

#define PORT 4321

char global_uuid[128] = {0};  // Enough space for a UUID string

struct PostData {
    char *data;
    size_t size;
};

int post_to_another_server(const char *uuid) {
    CURL *curl;
    CURLcode res;
    int result = 0;

    // JSON payload
    char post_data[256];
    snprintf(post_data, sizeof(post_data), "{\"uuid\":\"%s\", \"value\":\"ok\"}", uuid);

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to init curl\n");
        return -1;
    }

    // Headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8098"); // Change if needed
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  // Enable debug logging

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        result = -1;
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result;
}

void *post_uuid_thread(void *arg) {
    char *uuid = (char *)arg;

    // Wait a tiny bit to ensure the other server is ready
    sleep(3);

    post_to_another_server(uuid);
    free(uuid);  // Cleanup after thread

    return NULL;
}

static int send_response(struct MHD_Connection *connection, const char *response_text, int status_code) {
    struct MHD_Response *response;
    int ret;

    response = MHD_create_response_from_buffer(strlen(response_text), (void *)response_text, MHD_RESPMEM_PERSISTENT);
    if (!response) {
        fprintf(stderr, "Failed to create response\n");
        return MHD_NO;
    }

    ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);

    return ret;
}


static enum MHD_Result iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key, const char *filename,
                                    const char *content_type, const char *transfer_encoding, const char *data, uint64_t offset, size_t size) {
    struct PostData *post_data = (struct PostData *)coninfo_cls;

    if (size > 0) {
        post_data->data = realloc(post_data->data, post_data->size + size + 1);
        if (!post_data->data) return MHD_NO;

        memcpy(post_data->data + post_data->size, data, size);
        post_data->size += size;
        post_data->data[post_data->size] = '\0';
    }

    return MHD_YES;
}

static void request_completed_callback(void *cls, struct MHD_Connection *connection, void **con_cls,
                                       enum MHD_RequestTerminationCode toe) {
    struct PostData *post_data = *con_cls;
    if (post_data) {
        free(post_data->data);
        free(post_data);
    }
}

static enum MHD_Result request_handler(void *cls, struct MHD_Connection *connection, const char *url,
                                       const char *method, const char *version, const char *upload_data,
                                       size_t *upload_data_size, void **con_cls) {
    if (strcmp(url, "/boostrapping") != 0) {
      	printf("Invalid endpoint requested: %s\n", url);
        return send_response(connection, "Invalid endpoint\n", MHD_HTTP_NOT_FOUND);
    }
    
    struct PostData *post_data;

    if (strcmp(method, "POST") != 0) {
        printf("Unsupported method: %s\n", method);
        return send_response(connection, "Only POST method is supported", MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    if (!*con_cls) {
        post_data = malloc(sizeof(struct PostData));
        if (!post_data) return MHD_NO;

        post_data->data = NULL;
        post_data->size = 0;
        *con_cls = post_data;

        printf("Initialized POST data handler\n");
        return MHD_YES;
    }

    post_data = *con_cls;

    if (*upload_data_size != 0) {
        post_data->data = realloc(post_data->data, post_data->size + *upload_data_size + 1);
        if (!post_data->data) return MHD_NO;

        memcpy(post_data->data + post_data->size, upload_data, *upload_data_size);
        post_data->size += *upload_data_size;
        post_data->data[post_data->size] = '\0';
        *upload_data_size = 0;

        return MHD_YES;
    } else {

    // Parse JSON
    cJSON *json = cJSON_Parse(post_data->data);
    if (!json) {
        fprintf(stderr, "Invalid JSON received\n");
        return send_response(connection, "Invalid JSON\n", MHD_HTTP_BAD_REQUEST);
    }

    // Extract fields
    const cJSON *uuid = cJSON_GetObjectItemCaseSensitive(json, "uuid");
    const cJSON *device = cJSON_GetObjectItemCaseSensitive(json, "device");
    const cJSON *ip_address = cJSON_GetObjectItemCaseSensitive(json, "ip_address");
    const cJSON *mud_url = cJSON_GetObjectItemCaseSensitive(json, "mud-url");

    if (!cJSON_IsString(uuid) || !cJSON_IsString(device) || !cJSON_IsString(ip_address) || !cJSON_IsString(mud_url)) {
        cJSON_Delete(json);
        fprintf(stderr, "Missing or invalid fields in JSON\n");
        return send_response(connection, "Missing or invalid fields\n", MHD_HTTP_BAD_REQUEST);
    }

    // Print extracted values
    printf("UUID: %s\n", uuid->valuestring);
    printf("Device: %s\n", device->valuestring);
    printf("IP Address: %s\n", ip_address->valuestring);
    printf("MUD URL: %s\n", mud_url->valuestring);

    strncpy(global_uuid, uuid->valuestring, sizeof(global_uuid) - 1);
    global_uuid[sizeof(global_uuid) - 1] = '\0';  // Ensure null-termination
        
    // Copy UUID into heap memory for thread
    char *uuid_copy = strdup(global_uuid);
    if (uuid_copy) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, post_uuid_thread, uuid_copy) != 0) {
            fprintf(stderr, "Failed to create background thread\n");
            free(uuid_copy);
        } else {
            pthread_detach(thread);  // No need to join
        }
    }

    cJSON_Delete(json); // Cleanup JSON object

    return send_response(connection, "Data processed successfully\n", MHD_HTTP_OK);
}


}

int main() {
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, request_handler, NULL,
                              MHD_OPTION_NOTIFY_COMPLETED, request_completed_callback, NULL, MHD_OPTION_END);
    if (!daemon) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }

    printf("Server is running on http://localhost:%d/boostrapping\n", PORT);
    getchar(); // Press Enter to stop the server

    MHD_stop_daemon(daemon);
    curl_global_cleanup();
    return 0;
}
