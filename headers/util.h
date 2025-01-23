#ifndef _UTIL_H_
#define _UTIL_H_

#define FAILURE -1

/* Util to display the error message for the specified error code  */
static int app_perror(const char *sender, const char *title, pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));

    PJ_LOG(3, (sender, "%s: %s [code=%d]", title, errmsg, status));
    return 1;
}

#endif /* _UTIL_H_ */
