
#include "globus_common.h"
#include "globus_xio_system.h"

#define GlobusLXIOAllocOperation(_op_info)                                  \
    do                                                                      \
    {                                                                       \
        globus_l_operation_info_t *     _l_op_info;                         \
                                                                            \
        _l_op_info = (globus_l_operation_info_t *)                          \
            globus_memory_pop_node(&globus_l_operation_info_memory);        \
        if(_l_op_info)                                                      \
        {                                                                   \
            memset(_l_op_info, 0, sizeof(globus_l_operation_info_t));       \
        }                                                                   \
        (_op_info) = _l_op_info;                                            \
    } while(0)

#define GlobusLXIOFreeOperation(_op_info)                                   \
    (globus_memory_push_node(&globus_l_operation_info_memory, (_op_info)))

#define GlobusLXIOAllocIovec(_count, _iovec)                                \
    do                                                                      \
    {                                                                       \
        if((_count) < 10)                                                   \
        {                                                                   \
            (_iovec) = (struct iovec *)                                     \
                globus_memory_pop_node(&globus_l_iov_memory);               \
        }                                                                   \
        else                                                                \
        {                                                                   \
            (_iovec) = (struct iovec *)                                     \
                globus_malloc(sizeof(struct iovec) * iovc);                 \
        }                                                                   \
    } while(0)

#define GlobusLXIOFreeIovec(_count, _iovec)                                 \
    do                                                                      \
    {                                                                       \
        if((_count) < 10)                                                   \
        {                                                                   \
            globus_memory_push_node(&globus_l_iov_memory, (_iovec));        \
        }                                                                   \
        else                                                                \
        {                                                                   \
            globus_free((_iovec));                                          \
        }                                                                   \
    } while(0)

#define GlobusLXIOAllocMsghdr(_msghdr)                                      \
    do                                                                      \
    {                                                                       \
        struct msghdr *                 _l_msghdr;                          \
                                                                            \
        _l_msghdr = (struct msghdr *)                                       \
            globus_memory_pop_node(&globus_l_msghdr_memory);                \
        if(_l_msghdr)                                                       \
        {                                                                   \
            memset(_l_msghdr, 0, sizeof(struct msghdr));                    \
        }                                                                   \
        (_msghdr) = _l_msghdr;                                              \
    } while(0)

#define GlobusLXIOFreeMsghdr(_msghdr)                                       \
    (globus_memory_push_node(&globus_l_msghdr_memory, (_msghdr)))

#define GlobusLXIOTransferIovec(_iovec, _xiovec, _iovc)                     \
    do                                                                      \
    {                                                                       \
        int                             i;                                  \
                                                                            \
        for(i = 0; i < (_iovc); i++)                                        \
        {                                                                   \
            (_iovec)[i].iov_base = (_xiovec)[i].iov_base;                   \
            (_iovec)[i].iov_len = (_xiovec)[i].iov_len;                     \
        }                                                                   \
    } while(0)

#define GlobusLXIOCloseFd(_fd)                                              \
    do                                                                      \
    {                                                                       \
        int                             _rc;                                \
                                                                            \
        do                                                                  \
        {                                                                   \
            rc = close((_fd));                                              \
        } while(rc < 0 && errno == EINTR);                                  \
                                                                            \
        (_fd) = -1;                                                         \
    } while(0)

#define GlobusLXIOAddNonBlocking(_fd, _rc)                                  \
    do                                                                      \
    {                                                                       \
        int                         _l_fd;                                  \
        int                         _l_flags;                               \
                                                                            \
        _l_fd = (_fd);                                                      \
        _l_flags = fcntl(_l_fd, F_GETFL);                                   \
        if(_l_flags < 0)                                                    \
        {                                                                   \
            (_rc) = _l_flags;                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
             _l_flags |= O_NONBLOCK;                                        \
            (_rc) = fcntl(_l_fd, F_SETFL, _l_flags);                        \
        }                                                                   \
    } while(0)                                                              
                                                                            
#define GlobusLXIORemoveNonBlocking(_fd, _rc)                               \
    do                                                                      \
    {                                                                       \
        int                         _l_fd;                                  \
        int                         _l_flags;                               \
                                                                            \
        _l_fd = (_fd);                                                      \
        _l_flags = fcntl(_l_fd, F_GETFL);                                   \
        if(_l_flags < 0)                                                    \
        {                                                                   \
            (_rc) = _l_flags;                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
             _l_flags &= ~O_NONBLOCK;                                       \
            (_rc) = fcntl(_l_fd, F_SETFL, _l_flags);                        \
        }                                                                   \
    } while(0)

#define GlobusLAdjustIovec(_iov, _iovc, _nbytes)                            \
    do                                                                      \
    {                                                                       \
        globus_ssize_t                  _n;                                 \
        struct iovec *                  _l_iov;                             \
        int                             _l_iovc;                            \
        int                             _i;                                 \
                                                                            \
        _n = (_nbytes);                                                     \
        _l_iov = (_iov);                                                    \
        _l_iovc = (_iovc);                                                  \
                                                                            \
        /* skip all completely filled iovecs */                             \
        for(_i = 0, _n = rc;                                                \
            _i < _l_iovc &&  _n >= _l_iov[_i].iov_len;                      \
            _n -= _l_iov[_i].iov_len, _i++);                                \
                                                                            \
        if(_i < _l_iovc)                                                    \
        {                                                                   \
            _l_iov[_i].iov_base = (char *) _l_iov[_i].iov_base + _n;        \
            _l_iov[_i].iov_len -= _n;                                       \
            (_iov) += _i;                                                   \
            (_iovc) -= _i;                                                  \
        }                                                                   \
    } while(0)

typedef enum
{
    GLOBUS_L_OPERATION_ACCEPT,
    GLOBUS_L_OPERATION_CONNECT,
    GLOBUS_L_OPERATION_OPEN,
    GLOBUS_L_OPERATION_READ,
    GLOBUS_L_OPERATION_READV,
    GLOBUS_L_OPERATION_RECV,
    GLOBUS_L_OPERATION_RECVFROM,
    GLOBUS_L_OPERATION_RECVMSG,
    GLOBUS_L_OPERATION_WRITE,
    GLOBUS_L_OPERATION_WRITEV,
    GLOBUS_L_OPERATION_SEND,
    GLOBUS_L_OPERATION_SENDTO,
    GLOBUS_L_OPERATION_SENDMSG
} globus_l_operation_type_t;

#define _op_nbytes          op.data.nbytes
#define _op_single          op.data.buf.single
#define _op_iovecCom        op.data.buf.iovec
#define _op_iovec           op.data.buf.iovec.cont.plain
#define _op_msg             op.data.buf.iovec.cont.ex

typedef struct
{
    /* common members */
    globus_l_operation_type_t                     type;
    int                                           fd;
    globus_result_t                               result;
    void *                                        user_arg;
    
    union
    {
        /* non data ops -- open, connect, accept */
        struct
        {
            globus_xio_system_callback_t          callback;
            int *                                 out_fd;
        } non_data;
        
        /* data ops */
        struct
        {
            globus_xio_system_data_callback_t     callback;
            globus_ssize_t                        nbytes;
            globus_size_t                         waitforbytes;
            
            union
            {
                /* single buffer ops -- read, recv[from], write, send[to] */
                struct
                {
                    void *                        buf;
                    globus_size_t                 bufsize;
                    
                    /* extra data used for recv[from] and send[to] */
                    struct
                    {
                        const globus_sockaddr_t * addr;
                        int                       flags;
                    } ex;
                } single;
                
                /* ops involving iovecs  -- readv, writev, recvmsg, sendmsg */
                struct
                {
                    struct iovec *                start_iov;
                    int                           start_iovc;
                    
                    union
                    {
                        /* for readv and writev */
                        struct
                        {
                            struct iovec *        iov;
                            int                   iovc;
                        } plain;
                        
                        /* for recvmsg and sendmsg */
                        struct
                        {
                            struct msghdr *       msghdr;
                            int                   flags;
                        } ex;
                    } cont;
                } iovec;
            } buf;
        } data;
    } op;
} globus_l_operation_info_t;

static globus_cond_t                    globus_l_cond;
static globus_mutex_t                   globus_l_fdset_mutex;
static globus_mutex_t                   globus_l_cancel_mutex;
static globus_bool_t                    globus_l_select_active;
static globus_bool_t                    globus_l_wakeup_pending;
static globus_bool_t                    globus_l_shutdown_called;
static int                              globus_l_highest_fd;
static int                              globus_l_max_allowed_fds;
static fd_set *                         globus_l_read_fds;
static fd_set *                         globus_l_write_fds;
static fd_set *                         globus_l_ready_reads;
static fd_set *                         globus_l_ready_writes;
static fd_set *                         globus_l_canceled_reads;
static fd_set *                         globus_l_canceled_writes;
static globus_memory_t                  globus_l_operation_info_memory;
static globus_l_operation_info_t *      globus_l_read_operations;
static globus_l_operation_info_t *      globus_l_write_operations;
static globus_memory_t                  globus_l_iov_memory;
static globus_memory_t                  globus_l_msghdr_memory;
static int                              globus_l_wakeup_pipe[2];


 /* In the pre-activation of the thread module, we
     * are setting up some code to block the SIGPIPE
     * signal. This is necessary because some of
     * the TCP protocols we are using do not have
     * a mode in which we can safely detect a remotely-
     * closing socket.
     */
     
globus_result_t
globus_xio_system_open(
    const char *                        pathname,
    int                                 flags,
    int                                 mode,
    globus_xio_system_handle_t *        out_fd)
{
    int                                 fd;
    globus_result_t                     result;
    
    do
    {
        fd = open(pathname, flags, mode);
    } while(fd < 0 && errno == EINTR);
    
    *out_fd = fd;
    result = GLOBUS_SUCCESS;
    if(fd < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_open", errno);
    }
    
    return result;
}

globus_result_t
globus_xio_system_register_open(
    const char *                        pathname,
    int                                 flags,
    int                                 mode,
    globus_xio_system_handle_t *        out_fd,
    globus_xio_system_callback_t        callback,
    void *                              user_arg)
{
    int                                 fd;
    globus_result_t                     result;
    
    do
    {
        fd = open(pathname, flags | O_NONBLOCK, mode);
    } while(fd < 0 && errno == EINTR);
    
    if(fd < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_open", errno);
    }
    else
    {
        globus_l_operation_info_t *     op_info;
        
        GlobusLXIOAllocOperation(op_info);
        if(!op_info)
        {
            GlobusLXIOCloseFd(fd);
            result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                "globus_xio_system_register_open", "op_info");
        }
        else
        {
            op_info->type = GLOBUS_L_OPERATION_OPEN;
            op_info->fd = fd;
            op_info->user_arg = user_arg;
            op_info->op.non_data.callback = callback;
            
            if(flags & GLOBUS_XIO_SYSTEM_RDONLY)
            {
                result = globus_l_register_read(fd, op_info);
            }
            else
            {
                result = globus_l_register_write(fd, op_info);
            }
            
            if(result != GLOBUS_SUCCESS)
            {
                GlobusLXIOCloseFd(fd);
                GlobusLXIOFreeOperation(op_info);
            }
        }
    }
    
    *out_fd = fd;
    return result;
}

globus_result_t
globus_xio_system_connect(
    globus_xio_system_handle_t          fd,
    const globus_sockaddr_t *           addr)
{
    globus_result_t                     result;
    int                                 rc;
    
    do
    {
        rc = connect(
            fd, (const struct sockaddr *) addr, sizeof(globus_sockaddr_t));
    } while(rc < 0 && rc == EINTR);
    
    result = GLOBUS_SUCCESS;
    if(rc < 0)
    {
        GlobusLXIOCloseFd(fd);
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_connect", errno);
    }

    return result;
}

globus_result_t
globus_xio_system_register_connect(
    globus_xio_system_handle_t          fd,
    const globus_sockaddr_t *           addr,
    globus_xio_system_callback_t        callback,
    void *                              user_arg)
{
    globus_bool_t                       done;
    int                                 rc;
            
    GlobusLXIOAddNonBlocking(fd, rc);
    if(rc < 0)
    {
        return GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_register_connect", errno);
    }
    
    done = GLOBUS_FALSE;
    result = GLOBUS_SUCCESS;
    while(!done && connect(
        fd, (const struct sockaddr *) addr, sizeof(globus_sockaddr_t)) < 0)
    {
        switch(errno)
        {
          case EINPROGRESS:
            done = GLOBUS_TRUE;
            break;
            
          case EINTR:
            /* retry */
            break;
            
          case ETIMEDOUT: /* this was in globus io.. not sure why */
            globus_thread_yield();
            break;
            
          default:
            done = GLOBUS_TRUE;
            result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
                "globus_xio_system_register_connect", errno);
            break;
        }
    }
    
    if(result == GLOBUS_SUCCESS)
    {
        globus_l_operation_info_t *     op_info;
        
        GlobusLXIOAllocOperation(op_info);
        if(!op_info)
        {
            GlobusLXIOCloseFd(fd);
            result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                "globus_xio_system_register_connect", "op_info");
        }
        else
        {
            op_info->type = GLOBUS_L_OPERATION_CONNECT;
            op_info->fd = fd;
            op_info->user_arg = user_arg;
            op_info->op.non_data.callback = callback;
            
            result = globus_l_register_write(fd, op_info);
            
            if(result != GLOBUS_SUCCESS)
            {
                GlobusLXIOFreeOperation(op_info);
            }
        }
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        GlobusLXIOCloseFd(fd);
    }
    
    return result;
}

globus_result_t
globus_xio_system_accept(
    globus_xio_system_handle_t          listener_fd,
    globus_xio_system_handle_t *        out_fd)
{
    globus_result_t                     result;
    globus_sockaddr_t                   addr;
    int                                 addrlen;
    int                                 new_fd;
    
    addrlen = sizeof(globus_sockaddr_t);
    
    do
    {
        new_fd = accept(listener_fd, &addr, &addrlen);
    } while(new_fd < 0 && errno == EINTR);
    
    result = GLOBUS_SUCCESS;
    if(new_fd < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_accept", errno);
    }
    
    *out_fd = new_fd;

    return result;
}

globus_result_t
globus_xio_system_register_accept(
    globus_xio_system_handle_t          listener_fd,
    globus_xio_system_handle_t *        out_fd,
    globus_xio_system_callback_t        callback,
    void *                              user_arg)
{
    globus_result_t                     result;
    globus_l_operation_info_t *         op_info;
        
    GlobusLXIOAllocOperation(op_info);
    if(!op_info)
    {
        result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
            "globus_xio_system_register_accept", "op_info");
    }
    else
    {
        op_info->type = GLOBUS_L_OPERATION_ACCEPT;
        op_info->fd = listener_fd;
        op_info->user_arg = user_arg;
        op_info->op.non_data.callback = callback;
        op_info->op.non_data.out_fd = out_fd;
        
        result = globus_l_register_read(listener_fd, op_info);
        
        if(result != GLOBUS_SUCCESS)
        {
            GlobusLXIOFreeOperation(op_info);
        }
    }
        
    return result;
}

globus_result_t
globus_xio_system_read(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    globus_ssize_t *                    nread)
{
    globus_result_t                     result;
    globus_ssize_t                      nybtes;
    globus_ssize_t                      rc;
    int                                 save_errno;
    
    result = GLOBUS_SUCCESS;
    nybtes = 0;
    rc = 1;
    
    if(u_iovc == 1)
    {
        do
        {
            rc = read(
                fd,
                (char *) u_iov->iov_base + nbytes,
                u_iov->iov_len - nbytes);
            if(rc > 0)
            {
                nbytes += rc;
            }
        } while((rc > 0 && nbytes < waitforbytes) || 
            (rc < 0 && errno == EINTR));
        
        save_errno = errno;
    }
    else
    {
        struct iovec *                  iov;
        int                             iovc;
        
        GlobusLXIOAllocIovec(u_iovc, iov);
        
        if(!iov)
        {
            result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                "globus_xio_system_read", "iov");
        }
        else
        {
            struct iovec *              iov_save;
            
            iov_save = iov;
            iovc = u_iovc;
            GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
            
            do
            {
                rc = readv(fd, iov, iovc);
                if(rc > 0)
                {
                    nbytes += rc;
                    GlobusLAdjustIovec(iov, iovc, rc);
                }
            } while((rc > 0 && nbytes < waitforbytes) || 
                (rc < 0 && errno == EINTR));
            
            save_errno = errno;
            GlobusLXIOFreeIovec(u_iovc, iov_save);
        }
    }
    
    if(rc == 0)
    {
        result = globus_xio_driver_construct_eof();
    }
    else if(rc < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_read", save_errno);
    }
    
    *nread = nbytes;
    return result;
}

globus_result_t
globus_xio_system_read_ex(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    int                                 flags,
    const globus_sockaddr_t *           from,
    globus_ssize_t *                    nread)
{
    globus_result_t                     result;
    globus_ssize_t                      nybtes;
    globus_ssize_t                      rc;
    int                                 save_errno;
    
    if(!flags && !from)
    {
        return globus_xio_system_read(fd, u_iov, u_iovc, nread);
    }
    
    result = GLOBUS_SUCCESS;
    nybtes = 0;
    rc = 1;
    
    if(u_iovc == 1)
    {
        if(from)
        {
            do
            {
                rc = recvfrom(
                    fd,
                    (char *) u_iov->iov_base + nbytes,
                    u_iov->iov_len - nbytes,
                    flags,
                    from,
                    sizeof(globus_sockaddr_t));
                if(rc > 0)
                {
                    nbytes += rc;
                }
            } while((rc > 0 && nbytes < waitforbytes) || 
                (rc < 0 && errno == EINTR));
        }
        else
        {
            do
            {
                rc = recv(
                    fd,
                    (char *) u_iov->iov_base + nbytes,
                    u_iov->iov_len - nbytes,
                    flags);
                if(rc > 0)
                {
                    nbytes += rc;
                }
            } while((rc > 0 && nbytes < waitforbytes) || 
                (rc < 0 && errno == EINTR));
        }
        
        save_errno = errno;
    }
    else
    {
        struct msghdr *                 msghdr;                  
        struct iovec *                  iov;
        int                             iovc;
        
        GlobusLXIOAllocIovec(u_iovc, iov);
        
        if(!iov)
        {
            result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                "globus_xio_system_read_ex", "iov");
        }
        else
        {
            GlobusLXIOAllocMsghdr(msghdr);
            if(!msghdr)
            {
                GlobusLXIOFreeIovec(u_iovc, iov);
                result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                    "globus_xio_system_read_ex", "msghdr");
            }
        }
        
        if(result == GLOBUS_SUCCESS)
        {
            struct iovec *              iov_save;
            
            iov_save = iov;
            iovc = u_iovc;
            GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
            
            msghdr->msg_name = from;
            msghdr->msg_namelen = sizeof(globus_sockaddr_t);
            
            do
            {
                msghdr->msg_iov = iov;
                msghdr->msg_iovlen = iovc;
                
                rc = recvmsg(fd, msghdr, flags);
                if(rc > 0)
                {
                    nbytes += rc;
                    GlobusLAdjustIovec(iov, iovc, rc);
                }
            } while((rc > 0 && nbytes < waitforbytes) || 
                (rc < 0 && errno == EINTR));
            
            save_errno = errno;
            GlobusLXIOFreeMsghdr(msghdr);
            GlobusLXIOFreeIovec(u_iovc, iov_save);
        }
    }
    
    if(rc == 0)
    {
        result = globus_xio_driver_construct_eof();
    }
    else if(rc < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_read", save_errno);
    }
    
    *nread = nbytes;
    return result;
}
    
globus_result_t
globus_xio_system_register_read(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    globus_xio_system_data_callback_t   callback,
    void *                              user_arg)
{
    globus_result_t                     result;
    globus_l_operation_info_t *         op_info;
    
    result = GLOBUS_SUCCESS;
    GlobusLXIOAllocOperation(op_info);
    if(!op_info)
    {
        result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
            "globus_xio_system_register_read", "op_info");
    }
    else
    {
        struct iovec *                  iov;
        
        if(u_iovc == 1)
        {
            op_info->type = GLOBUS_L_OPERATION_READ;
            op_info->_op_single.buf = u_iov->iov_base;
            op_info->_op_single.bufsize = u_iov->iov_len;
        }
        else
        {
            GlobusLXIOAllocIovec(u_iovc, iov);
            if(!iov)
            {
                result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                    "globus_xio_system_register_read", "iov");
            }
            else
            {
                GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
                
                op_info->type = GLOBUS_L_OPERATION_READV;
                op_info->_op_iovecCom.start_iov = iov;
                op_info->_op_iovec.iov = iov;
                op_info->_op_iovecCom.start_iovc = u_iovc;
                op_info->_op_iovec.iovc = u_iovc;
            }
        }
        
        if(result == GLOBUS_SUCCESS)
        {
            op_info->fd = fd;
            op_info->user_arg = user_arg;
            op_info->op.data.callback = callback;
            op_info->op.data.waitforbytes = waitforbytes;
        
            result = globus_l_register_read(fd, op_info);
            if(result != GLOBUS_SUCCESS)
            {
                if(u_iovc != 1)
                {
                    GlobusLXIOFreeIovec(u_iovc, iov);
                }
                
                GlobusLXIOFreeOperation(op_info);
            }
        }
        else
        {
            GlobusLXIOFreeOperation(op_info);
        }
    }
        
    return result;
}

globus_result_t
globus_xio_system_register_read_ex(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    int                                 flags,
    const globus_sockaddr_t *           from,
    globus_xio_system_data_callback_t   callback,
    void *                              user_arg)
{
    globus_result_t                     result;
    globus_l_operation_info_t *         op_info;
    
    if(!flags && !from)
    {
        return globus_xio_system_register_read(
            fd, u_iov, u_iovc, callback, user_arg);
    }
    
    result = GLOBUS_SUCCESS;
    GlobusLXIOAllocOperation(op_info);
    if(!op_info)
    {
        result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
            "globus_xio_system_register_read_ex", "op_info");
    }
    else
    {
        struct iovec *                  iov;
        struct msghdr *                 msghdr;
        
        if(u_iovc == 1)
        {
            if(from)
            {
                op_info->type = GLOBUS_L_OPERATION_READFROM;
                op_info->_op_single.ex.addr = from;
            }
            else
            {
                op_info->type = GLOBUS_L_OPERATION_RECV;
            }
            
            op_info->_op_single.buf = u_iov->iov_base;
            op_info->_op_single.bufsize = u_iov->iov_len;
            op_info->_op_single.ex.flags = flags;
        }
        else
        {
            GlobusLXIOAllocIovec(u_iovc, iov);
            if(!iov)
            {
                result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                    "globus_xio_system_register_read_ex", "iov");
            }
            else
            {
                GlobusLXIOAllocMsghdr(msghdr);
                if(!msghdr)
                {
                    GlobusLXIOFreeIovec(u_iovc, iov);
                    result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                        "globus_xio_system_register_read_ex", "msghdr");
                }
            }
            
            if(result == GLOBUS_SUCCESS)
            {
                GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
                msghdr->msg_name = from;
                msghdr->msg_namelen = sizeof(globus_sockaddr_t);
                msghdr->msg_iov = iov;
                msghdr->msg_iovlen = u_iovc;
                
                op_info->type = GLOBUS_L_OPERATION_READMSG;
                op_info->_op_iovecCom.start_iov = iov;
                op_info->_op_iovecCom.start_iovc = u_iovc;
                op_info->_op_msg.msghdr = msghdr;
                op_info->_op_msg.flags = flags;
            }
        }
        
        if(result == GLOBUS_SUCCESS)
        {
            op_info->fd = fd;
            op_info->user_arg = user_arg;
            op_info->op.data.callback = callback;
            op_info->op.data.waitforbytes = waitforbytes;
        
            result = globus_l_register_read(fd, op_info);
            if(result != GLOBUS_SUCCESS)
            {
                if(u_iovc != 1)
                {
                    GlobusLXIOFreeMsghdr(msghdr);
                    GlobusLXIOFreeIovec(u_iovc, iov);
                }
                
                GlobusLXIOFreeOperation(op_info);
            }
        }
        else
        {
            GlobusLXIOFreeOperation(op_info);
        }
    }
        
    return result;
}

globus_result_t
globus_xio_system_write(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    globus_ssize_t *                    nwritten)
{
    globus_result_t                     result;
    globus_ssize_t                      nybtes;
    globus_ssize_t                      rc;
    int                                 save_errno;
    
    result = GLOBUS_SUCCESS;
    nybtes = 0;
    rc = 1;
    
    if(u_iovc == 1)
    {
        do
        {
            rc = write(
                fd,
                (char *) u_iov->iov_base + nbytes,
                u_iov->iov_len - nbytes);
            if(rc > 0)
            {
                nbytes += rc;
            }
        } while((rc >= 0 && nbytes < u_iov->iov_len) || 
            (rc < 0 && errno == EINTR));
        
        save_errno = errno;
    }
    else
    {
        struct iovec *                  iov;
        int                             iovc;
        
        GlobusLXIOAllocIovec(u_iovc, iov);
        
        if(!iov)
        {
            result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                "globus_xio_system_write", "iov");
        }
        else
        {
            struct iovec *              iov_save;
            
            iov_save = iov;
            iovc = u_iovc;
            GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
            
            do
            {
                rc = writev(fd, iov, iovc);
                if(rc > 0)
                {
                    nbytes += rc;
                    GlobusLAdjustIovec(iov, iovc, rc);
                }
            } while((rc >= 0 && iovc > 0) || 
                (rc < 0 && errno == EINTR));
            
            save_errno = errno;
            GlobusLXIOFreeIovec(u_iovc, iov_save);
        }
    }
    
    if(rc < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_write", save_errno);
    }
    
    *nwritten = nbytes;
    return result;
}

globus_result_t
globus_xio_system_write_ex(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    int                                 flags,
    const globus_sockaddr_t *           to,
    globus_ssize_t *                    nwritten)
{
    globus_result_t                     result;
    globus_ssize_t                      nybtes;
    globus_ssize_t                      rc;
    int                                 save_errno;
    
    if(!flags && !to)
    {
        return globus_xio_system_write(fd, u_iov, u_iovc, nwritten);
    }
    
    result = GLOBUS_SUCCESS;
    nybtes = 0;
    rc = 1;
    
    if(u_iovc == 1)
    {
        if(to)
        {
            do
            {
                rc = sendto(
                    fd,
                    (char *) u_iov->iov_base + nbytes,
                    u_iov->iov_len - nbytes,
                    flags,
                    to,
                    sizeof(globus_sockaddr_t));
                if(rc > 0)
                {
                    nbytes += rc;
                }
            } while((rc >= 0 && nbytes < u_iov->iov_len) || 
                (rc < 0 && errno == EINTR));
        }
        else
        {
            do
            {
                rc = send(
                    fd,
                    (char *) u_iov->iov_base + nbytes,
                    u_iov->iov_len - nbytes,
                    flags);
                if(rc > 0)
                {
                    nbytes += rc;
                }
            } while((rc >= 0 && nbytes < u_iov->iov_len) || 
                (rc < 0 && errno == EINTR));
        }
        
        save_errno = errno;
    }
    else
    {
        struct msghdr *                 msghdr;                  
        struct iovec *                  iov;
        int                             iovc;
        
        GlobusLXIOAllocIovec(u_iovc, iov);
        
        if(!iov)
        {
            result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                "globus_xio_system_write_ex", "iov");
        }
        else
        {
            GlobusLXIOAllocMsghdr(msghdr);
            if(!msghdr)
            {
                GlobusLXIOFreeIovec(u_iovc, iov);
                result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                    "globus_xio_system_write_ex", "msghdr");
            }
        }
        
        if(result == GLOBUS_SUCCESS)
        {
            struct iovec *              iov_save;
            
            iov_save = iov;
            iovc = u_iovc;
            GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
            
            msghdr->msg_name = to;
            msghdr->msg_namelen = sizeof(globus_sockaddr_t);
            
            do
            {
                msghdr->msg_iov = iov;
                msghdr->msg_iovlen = iovc;
                
                rc = sendmsg(fd, msghdr, flags);
                if(rc > 0)
                {
                    nbytes += rc;
                    GlobusLAdjustIovec(iov, iovc, rc);
                }
            } while((rc >= 0 && iovc > 0) || 
                (rc < 0 && errno == EINTR));
            
            save_errno = errno;
            GlobusLXIOFreeMsghdr(msghdr);
            GlobusLXIOFreeIovec(u_iovc, iov_save);
        }
    }
    
    if(rc < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_write_ex", save_errno);
    }
    
    *nwritten = nbytes;
    return result;
}
    
globus_result_t
globus_xio_system_register_write(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    globus_xio_system_data_callback_t   callback,
    void *                              user_arg)
{
    globus_result_t                     result;
    globus_l_operation_info_t *         op_info;
    
    result = GLOBUS_SUCCESS;
    GlobusLXIOAllocOperation(op_info);
    if(!op_info)
    {
        result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
            "globus_xio_system_register_write", "op_info");
    }
    else
    {
        struct iovec *                  iov;
        
        if(u_iovc == 1)
        {
            op_info->type = GLOBUS_L_OPERATION_WRITE;
            op_info->_op_single.buf = u_iov->iov_base;
            op_info->_op_single.bufsize = u_iov->iov_len;
        }
        else
        {
            GlobusLXIOAllocIovec(u_iovc, iov);
            if(!iov)
            {
                result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                    "globus_xio_system_register_write", "iov");
            }
            else
            {
                GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
                
                op_info->type = GLOBUS_L_OPERATION_WRITEV;
                op_info->_op_iovecCom.start_iov = iov;
                op_info->_op_iovec.iov = iov;
                op_info->_op_iovecCom.start_iovc = u_iovc;
                op_info->_op_iovec.iovc = u_iovc;
            }
        }
        
        if(result == GLOBUS_SUCCESS)
        {
            op_info->fd = fd;
            op_info->user_arg = user_arg;
            op_info->op.data.callback = callback;
            op_info->op.data.waitforbytes = waitforbytes;
        
            result = globus_l_register_write(fd, op_info);
            if(result != GLOBUS_SUCCESS)
            {
                if(u_iovc != 1)
                {
                    GlobusLXIOFreeIovec(u_iovc, iov);
                }
                
                GlobusLXIOFreeOperation(op_info);
            }
        }
        else
        {
            GlobusLXIOFreeOperation(op_info);
        }
    }
        
    return result;
}

globus_result_t
globus_xio_system_register_write_ex(
    globus_xio_system_handle_t          fd,
    const globus_xio_iovec_t *          u_iov,
    int                                 u_iovc,
    int                                 flags,
    const globus_sockaddr_t *           to,
    globus_xio_system_data_callback_t   callback,
    void *                              user_arg)
{
    globus_result_t                     result;
    globus_l_operation_info_t *         op_info;
    
    if(!flags && !to)
    {
        return globus_xio_system_register_write(
            fd, u_iov, u_iovc, callback, user_arg);
    }
    
    result = GLOBUS_SUCCESS;
    GlobusLXIOAllocOperation(op_info);
    if(!op_info)
    {
        result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
            "globus_xio_system_register_write_ex", "op_info");
    }
    else
    {
        struct iovec *                  iov;
        struct msghdr *                 msghdr;
        
        if(u_iovc == 1)
        {
            if(to)
            {
                op_info->type = GLOBUS_L_OPERATION_SENDTO;
                op_info->_op_single.ex.addr = to;
            }
            else
            {
                op_info->type = GLOBUS_L_OPERATION_SEND;
            }
            
            op_info->_op_single.buf = u_iov->iov_base;
            op_info->_op_single.bufsize = u_iov->iov_len;
            op_info->_op_single.ex.flags = flags;
        }
        else
        {
            GlobusLXIOAllocIovec(u_iovc, iov);
            if(!iov)
            {
                result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                    "globus_xio_system_register_write_ex", "iov");
            }
            else
            {
                GlobusLXIOAllocMsghdr(msghdr);
                if(!msghdr)
                {
                    GlobusLXIOFreeIovec(u_iovc, iov);
                    result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                        "globus_xio_system_register_write_ex", "msghdr");
                }
            }
            
            if(result == GLOBUS_SUCCESS)
            {
                GlobusLXIOTransferIovec(iov, u_iov, u_iovc);
                msghdr->msg_name = to;
                msghdr->msg_namelen = sizeof(globus_sockaddr_t);
                msghdr->msg_iov = iov;
                msghdr->msg_iovlen = u_iovc;
                
                op_info->type = GLOBUS_L_OPERATION_SENDMSG;
                op_info->_op_iovecCom.start_iov = iov;
                op_info->_op_iovecCom.start_iovc = u_iovc;
                op_info->_op_msg.msghdr = msghdr;
                op_info->_op_msg.flags = flags;
            }
        }
        
        if(result == GLOBUS_SUCCESS)
        {
            op_info->fd = fd;
            op_info->user_arg = user_arg;
            op_info->op.data.callback = callback;
            op_info->op.data.waitforbytes = waitforbytes;
        
            result = globus_l_register_write(fd, op_info);
            if(result != GLOBUS_SUCCESS)
            {
                if(u_iovc != 1)
                {
                    GlobusLXIOFreeMsghdr(msghdr);
                    GlobusLXIOFreeIovec(u_iovc, iov);
                }
                
                GlobusLXIOFreeOperation(op_info);
            }
        }
        else
        {
            GlobusLXIOFreeOperation(op_info);
        }
    }
        
    return result;
}

globus_result_t
globus_xio_system_close(
    globus_xio_system_handle_t          fd)
{
    globus_result_t                     result;
    int                                 rc;
    
    do
    {
        rc = close(fd);
    } while(rc < 0 && rc == EINTR);
    
    result = GLOBUS_SUCCESS;
    if(rc < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_close", errno);
    }

    return result;
}

typedef struct
{
    int                                 fd;
    globus_xio_system_callback_t        callback;
    void *                              user_arg;
} globus_l_close_info_t;

static
void
globus_l_close_kickout(
    void *                              user_arg)
{
    globus_l_close_info_t *             close_info;
    
    close_info = (globus_l_close_info_t *) user_arg;
    
    close_info->callback(close_info->fd, GLOBUS_SUCCESS, close_info->user_arg);
    
    globus_free(close_info);
}

globus_result_t
globus_xio_system_register_close(
    globus_xio_system_handle_t          fd,
    globus_xio_system_callback_t        callback,
    void *                              user_arg)
{
    globus_l_close_info_t *             close_info;
    globus_result_t                     result;
    int                                 rc;
    
    GlobusLXIOAddNonBlocking(fd, rc);
    
    do
    {
        rc = close(fd);
    } while(rc < 0 && rc == EINTR);
    
    if(rc < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_xio_system_register_close", errno);
    }
    else
    {
        close_info = (globus_l_close_info_t *)
            globus_malloc(sizeof(globus_l_close_info_t));
        if(!close_info)
        {
            result = GLOBUS_L_XIO_SYSTEM_CONSTRUCT_MEMORY_ALLOC(
                "globus_xio_system_register_close", "close_info");
        }
        else
        {
            close_info->fd = fd;
            close_info->callback = callback;
            close_info->user_arg = user_arg;
            
            result = globus_callback_register_oneshot(
                GLOBUS_NULL, GLOBUS_NULL, globus_l_close_kickout, close_info);
            if(result != GLOBUS_SUCCESS)
            {
                globus_free(close_info);
            }
        }
    }
    
    return result;
}

globus_result_t
globus_xio_system_cancel_open(
    globus_xio_system_handle_t          fd)
{
    globus_l_operation_info_t *         op_info;
    globus_bool_t                       read_op;
    globus_result_t                     result;
    
    op_info = GLOBUS_NULL;
    result = GLOBUS_SUCCESS;
    
    globus_mutex_lock(&globus_l_cancel_mutex);
    {
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            if(FD_ISSET(fd, globus_l_read_fds))
            {
                read_op = GLOBUS_TRUE;
                op_info = globus_l_read_operations[fd];
            }
            else if(FD_ISSET(fd, globus_l_write_fds))
            {
                read_op = GLOBUS_FALSE;
                op_info = globus_l_write_operations[fd];
            }
        
            if(!op_info || op_info->type != GLOBUS_L_OPERATION_OPEN)
            {
                result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_NOT_REGISTERED(
                    "globus_xio_system_cancel_open");
            }
            else
            {
                if((read_op && FD_ISSET(fd, globus_l_canceled_reads)) ||
                    (!read_op && FD_ISSET(fd, globus_l_canceled_writes)))
                {
                    result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                        "globus_xio_system_cancel_open");
                }
                else if(globus_l_select_active)
                {
                    /* pend the cancel for after select wakes up */
                    FD_SET(
                        fd, 
                        (read_op ? 
                            globus_l_canceled_reads : 
                            globus_l_canceled_writes));
                    if(!globus_l_wakeup_pending)
                    {
                        globus_l_select_wakeup();
                    }
                }
                else
                {
                    /* unregister and kickout now */
                    if(read_op)
                    {
                        globus_l_unregister_read(fd);
                    }
                    else
                    {
                        globus_l_unregister_write(fd);
                    }
                    
                    op_info->result = 
                        GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                            "globus_xio_system_cancel_open");
                            
                    result = globus_callback_register_oneshot(
                        GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, op_info);
                    globus_assert(result == GLOBUS_SUCCESS);
                }
            }
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
    }
    globus_mutex_unlock(&globus_l_cancel_mutex);
    
    return result;
}

globus_result_t
globus_xio_system_cancel_connect(
    globus_xio_system_handle_t          fd)
{
    globus_l_operation_info_t *         op_info;
    globus_result_t                     result;
    
    op_info = GLOBUS_NULL;
    result = GLOBUS_SUCCESS;
    
    globus_mutex_lock(&globus_l_cancel_mutex);
    {
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            if(FD_ISSET(fd, globus_l_write_fds))
            {
                op_info = globus_l_write_operations[fd];
            }
        
            if(!op_info || op_info->type != GLOBUS_L_OPERATION_CONNECT)
            {
                result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_NOT_REGISTERED(
                    "globus_xio_system_cancel_connect");
            }
            else
            {
                if(FD_ISSET(fd, globus_l_canceled_writes))
                {
                    result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                        "globus_xio_system_cancel_connect");
                }
                else if(globus_l_select_active)
                {
                    /* pend the cancel for after select wakes up */
                    FD_SET(fd, globus_l_canceled_writes);
                    if(!globus_l_wakeup_pending)
                    {
                        globus_l_select_wakeup();
                    }
                }
                else
                {
                    /* unregister and kickout now */
                    globus_l_unregister_write(fd);
                    op_info->result = 
                        GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                            "globus_xio_system_cancel_connect");
                            
                    result = globus_callback_register_oneshot(
                        GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, op_info);
                    globus_assert(result == GLOBUS_SUCCESS);
                }
            }
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
    }
    globus_mutex_unlock(&globus_l_cancel_mutex);
    
    return result;
}

globus_result_t
globus_xio_system_cancel_accept(
    globus_xio_system_handle_t          listener_fd)
{
    globus_l_operation_info_t *         op_info;
    globus_result_t                     result;
    
    op_info = GLOBUS_NULL;
    result = GLOBUS_SUCCESS;
    
    globus_mutex_lock(&globus_l_cancel_mutex);
    {
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            if(FD_ISSET(listener_fd, globus_l_read_fds))
            {
                op_info = globus_l_read_operations[listener_fd];
            }
        
            if(!op_info || op_info->type != GLOBUS_L_OPERATION_ACCEPT)
            {
                result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_NOT_REGISTERED(
                    "globus_xio_system_cancel_accept");
            }
            else
            {
                if(FD_ISSET(listener_fd, globus_l_canceled_reads))
                {
                    result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                        "globus_xio_system_cancel_accept");
                }
                else if(globus_l_select_active)
                {
                    /* pend the cancel for after select wakes up */
                    FD_SET(listener_fd, globus_l_canceled_reads);
                    if(!globus_l_wakeup_pending)
                    {
                        globus_l_select_wakeup();
                    }
                }
                else
                {
                    /* unregister and kickout now */
                    globus_l_unregister_read(listener_fd);
                    op_info->result = 
                        GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                            "globus_xio_system_cancel_accept");
                            
                    result = globus_callback_register_oneshot(
                        GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, op_info);
                    globus_assert(result == GLOBUS_SUCCESS);
                }
            }
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
    }
    globus_mutex_unlock(&globus_l_cancel_mutex);
    
    return result;
}

globus_result_t
globus_xio_system_cancel_read(
    globus_xio_system_handle_t          fd)
{
    globus_l_operation_info_t *         op_info;
    globus_result_t                     result;
    
    op_info = GLOBUS_NULL;
    result = GLOBUS_SUCCESS;
    
    globus_mutex_lock(&globus_l_cancel_mutex);
    {
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            if(FD_ISSET(fd, globus_l_read_fds))
            {
                op_info = globus_l_read_operations[fd];
            }
        
            if(!op_info || 
                !(op_info->type == GLOBUS_L_OPERATION_READ      ||
                op_info->type == GLOBUS_L_OPERATION_READV       ||
                op_info->type == GLOBUS_L_OPERATION_RECV        ||
                op_info->type == GLOBUS_L_OPERATION_RECVFROM    ||
                op_info->type == GLOBUS_L_OPERATION_RECVMSG))
            {
                result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_NOT_REGISTERED(
                    "globus_xio_system_cancel_read");
            }
            else
            {
                if(FD_ISSET(fd, globus_l_canceled_reads))
                {
                    result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                        "globus_xio_system_cancel_read");
                }
                else if(globus_l_select_active)
                {
                    /* pend the cancel for after select wakes up */
                    FD_SET(fd, globus_l_canceled_reads);
                    if(!globus_l_wakeup_pending)
                    {
                        globus_l_select_wakeup();
                    }
                }
                else
                {
                    /* unregister and kickout now */
                    globus_l_unregister_read(fd);
                    op_info->result = 
                        GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                            "globus_xio_system_cancel_read");
                            
                    result = globus_callback_register_oneshot(
                        GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, op_info);
                    globus_assert(result == GLOBUS_SUCCESS);
                }
            }
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
    }
    globus_mutex_unlock(&globus_l_cancel_mutex);
    
    return result;
}

globus_result_t
globus_xio_system_cancel_write(
    globus_xio_system_handle_t          fd)
{
    globus_l_operation_info_t *         op_info;
    globus_result_t                     result;
    
    op_info = GLOBUS_NULL;
    result = GLOBUS_SUCCESS;
    
    globus_mutex_lock(&globus_l_cancel_mutex);
    {
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            if(FD_ISSET(fd, globus_l_write_fds))
            {
                op_info = globus_l_write_operations[fd];
            }
        
            if(!op_info || 
                !(op_info->type == GLOBUS_L_OPERATION_WRITE ||
                op_info->type == GLOBUS_L_OPERATION_WRITEV  ||
                op_info->type == GLOBUS_L_OPERATION_SEND    ||
                op_info->type == GLOBUS_L_OPERATION_SENDTO  ||
                op_info->type == GLOBUS_L_OPERATION_SENDMSG))
            {
                result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_NOT_REGISTERED(
                    "globus_xio_system_cancel_write");
            }
            else
            {
                if(FD_ISSET(fd, globus_l_canceled_writes))
                {
                    result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                        "globus_xio_system_cancel_write");
                }
                else if(globus_l_select_active)
                {
                    /* pend the cancel for after select wakes up */
                    FD_SET(fd, globus_l_canceled_writes);
                    if(!globus_l_wakeup_pending)
                    {
                        globus_l_select_wakeup();
                    }
                }
                else
                {
                    /* unregister and kickout now */
                    globus_l_unregister_write(fd);
                    op_info->result = 
                        GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
                            "globus_xio_system_cancel_write");
                            
                    result = globus_callback_register_oneshot(
                        GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, op_info);
                    globus_assert(result == GLOBUS_SUCCESS);
                }
            }
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
    }
    globus_mutex_unlock(&globus_l_cancel_mutex);
    
    return result;
}

static
globus_result_t
globus_l_register_read(
    int                                 fd,
    globus_l_operation_info_t *         read_info)
{
    globus_result_t                     result;
    
    globus_mutex_lock(&globus_l_fdset_mutex);
    {
        if(fd >= globus_l_max_allowed_fds)
        {
            result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_TOO_MANY_FDS(
                "globus_l_register_read");
        }
        else if(FD_ISSET(fd, globus_l_read_fds))
        {
            result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_ALREADY_REGISTERED(
                "globus_l_register_read");
        }
        else
        {
            if(fd > globus_l_highest_fd)
            {
                globus_l_highest_fd = fd;
            }
            
            FD_SET(fd, globus_l_read_fds);
            globus_l_read_operations[fd] = read_info;
            
            if(globus_l_select_active && !globus_l_wakeup_pending)
            {
                globus_l_select_wakeup();
            }
            
            result = GLOBUS_SUCCESS;
        }
    }
    globus_mutex_unlock(&globus_l_fdset_mutex);
    
    return result;
}

static
globus_result_t
globus_l_register_write(
    int                                 fd,
    globus_l_operation_info_t *         write_info)
{
    globus_result_t                     result;
    
    globus_mutex_lock(&globus_l_fdset_mutex);
    {
        if(fd >= globus_l_max_allowed_fds)
        {
            result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_TOO_MANY_FDS(
                "globus_l_register_write");
        }
        else if(FD_ISSET(fd, globus_l_write_fds))
        {
            result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_ALREADY_REGISTERED(
                "globus_l_register_write");
        }
        else
        {
            if(fd > globus_l_highest_fd)
            {
                globus_l_highest_fd = fd;
            }
            
            FD_SET(fd, globus_l_write_fds);
            globus_l_write_operations[fd] = write_info;
            
            if(globus_l_select_active && !globus_l_wakeup_pending)
            {
                globus_l_select_wakeup();
            }
            
            result = GLOBUS_SUCCESS;
        }
    }
    globus_mutex_unlock(&globus_l_fdset_mutex);
    
    return result;
}

static
void
globus_l_unregister_read(
    int                                 fd)
{
    globus_assert(FD_ISSET(fd, globus_l_read_fds));
    FD_CLR(fd, globus_l_read_fds);
    globus_l_read_operations[fd] = GLOBUS_NULL;
}

static
void
globus_l_unregister_write(
    int                                 fd)
{
    globus_assert(FD_ISSET(fd, globus_l_write_fds));
    FD_CLR(fd, globus_l_write_fds);
    globus_l_write_operations[fd] = GLOBUS_NULL;
}

static
void
globus_l_kickout(
    void *                              user_arg)
{
    globus_l_operation_info_t *         op_info;
    
    op_info = (globus_l_operation_info_t *) user_arg;
    
    switch(op_info->type)
    {
      case GLOBUS_L_OPERATION_OPEN:
      case GLOBUS_L_OPERATION_CONNECT:
        /* first disable the non-blocking flag */
        if(op_info->result == GLOBUS_SUCCESS)
        {
            int                         rc;
            
            GlobusLXIORemoveNonBlocking(op_info->fd, rc);
            globus_assert(rc >= 0);
        }
        /* fall through */
      case GLOBUS_L_OPERATION_ACCEPT:
        op_info->op.non_data.callback(
            op_info->fd,
            op_info->result,
            op_info->user_arg);
        break;
      
      default:
        op_info->op.data.callback(
            op_info->fd,
            op_info->result,
            op_info->_op_nbytes,
            op_info->user_arg);
        
        switch(op_info->type)
        {
          case GLOBUS_L_OPERATION_RECVMSG:
          case GLOBUS_L_OPERATION_SENDMSG:
            GlobusLXIOFreeMsghdr(op_info->_op_msg.msghdr);
            
            /* fall through */
          case GLOBUS_L_OPERATION_READV:
          case GLOBUS_L_OPERATION_WRITEV:
            GlobusLXIOFreeIovec(
                op_info->_op_iovecCom.start_iovc,
                op_info->_op_iovecCom.start_iov);
            break;
            
          default:
            break;
        }  
            
        break;
    }
    
    GlobusLXIOFreeOperation(op_info);
}

static
void
globus_l_select_wakeup()
{
    int                                 rc;
    char                                byte;
    
    byte = 0;
    
    do
    {
        rc = write(globus_l_wakeup_pipe[1], &byte, sizeof(char));
    } while(rc < 0 && errno == EINTR);

    if(rc > 0)
    {
	globus_l_wakeup_pending = GLOBUS_TRUE;
    }
}

static
void
globus_l_handle_wakeup()
{
    char                                buf[64];
    globus_ssize_t                      done;

    do
    {
        done = read(globus_l_wakeup_pipe[0], buf, sizeof(buf));
    } while(done < 0 && errno == EINTR);

    globus_l_wakeup_pending = GLOBUS_FALSE;
}

static
globus_bool_t
globus_l_handle_read(
    int                                 fd)
{
    globus_bool_t                       handled_it;
    globus_l_operation_info_t *         read_info;
    globus_ssize_t                      nbytes;
    globus_result_t                     result;
    
    nbytes = 0;
    handled_it = GLOBUS_FALSE;
    read_info = globus_l_read_operations[fd];
    result = GLOBUS_SUCCESS;
    
    switch(read_info->type)
    {
      case GLOBUS_L_OPERATION_OPEN:
        nbytes = 1;  
        /* just so eof code doesnt trip -- nothing else to do here */
        break;
        
      case GLOBUS_L_OPERATION_READ:
        do
        {
            nbytes = read(
                fd,
                read_info->_op_single.buf,
                read_info->_op_single.buf.bufsize);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            read_info->_op_single.buf = (char *) 
                read_info->_op_single.buf + nbytes;
            read_info->_op_single.bufsize -= nbytes;
            read_info->_op_nbytes += nbytes;
        }
        break;
        
      case GLOBUS_L_OPERATION_READV:
        do
        {
            nbytes = readv(
                fd, read_info->_op_iovec.iov, read_info->_op_iovec.iovc);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            read_info->_op_nbytes += nbytes;
            GlobusLAdjustIovec(
                read_info->_op_iovec.iov, read_info->_op_iovec.iovc, nbytes);
        }
        break;
        
      case GLOBUS_L_OPERATION_RECV:
        do
        {
            nbytes = recv(
                fd,
                read_info->_op_single.buf,
                read_info->_op_single.bufsize,
                read_info->_op_single.ex.flags);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            read_info->_op_single.buf = (char *)
                read_info->_op_single.buf + nbytes;
            read_info->_op_single.bufsize -= nbytes;
            read_info->_op_nbytes += nbytes;
        }
        break;
        
      case GLOBUS_L_OPERATION_RECVFROM:
        do
        {
            nbytes = recvfrom(
                fd,
                read_info->_op_single.buf,
                read_info->_op_single.bufsize,
                read_info->_op_single.ex.flags,
                read_info->_op_single.ex.addr,
                sizeof(globus_sockaddr_t));
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            read_info->_op_single.buf = (char *)
                read_info->_op_single.buf + nbytes;
            read_info->_op_single.bufsize -= nbytes;
            read_info->_op_nbytes += nbytes;
        }
        break;
        
      case GLOBUS_L_OPERATION_RECVMSG:
        do
        {
            nbytes = recvmsg(
                fd, read_info->_op_msg.msghdr, read_info->_op_msg.flags);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            struct msghdr *             msghdr;
            
            read_info->_op_nbytes += nbytes;
            msghdr = read_info->_op_msg.msghdr;
            GlobusLAdjustIovec(msghdr->msg_iov, msghdr->msg_iovlen, nbytes);
        }
        break;
        
      case GLOBUS_L_OPERATION_ACCEPT:
      case GLOBUS_L_OPERATION_CONNECT:
      case GLOBUS_L_OPERATION_WRITE:
      case GLOBUS_L_OPERATION_WRITEV:
      case GLOBUS_L_OPERATION_SEND:
      case GLOBUS_L_OPERATION_SENDTO:
      case GLOBUS_L_OPERATION_SENDMSG:
      default:
        globus_assert(0 && "Unexpected type for read operation");
        return GLOBUS_FALSE;
        break;
    }
    
    if(nbytes < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_l_handle_read", errno);
    }
    else if(nbytes == 0)
    {
        result = globus_xio_driver_construct_eof();
    }
    
    if(read_info->_op_nbytes >= read_info->waitforbytes || 
        result != GLOBUS_SUCCESS)
    {
        handled_it = GLOBUS_TRUE;
        read_info->result = result;
        
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            globus_l_unregister_read(fd);
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
        
        result = globus_callback_register_oneshot(
            GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, read_info);
        globus_assert(result == GLOBUS_SUCCESS);
    }
    
    return handled_it;
}

static
globus_bool_t
globus_l_handle_write(
    int                                 fd)
{
    globus_bool_t                       handled_it;
    globus_l_operation_info_t *         write_info;
    globus_ssize_t                      nbytes;
    globus_result_t                     result;
    
    nbytes = 0;
    handled_it = GLOBUS_FALSE;
    result = GLOBUS_SUCCESS;
    write_info = globus_l_write_operations[fd];
    
    switch(write_info->type)
    {
      case GLOBUS_L_OPERATION_OPEN:
        /* nothing to do here */
        break;
      
      case GLOBUS_L_OPERATION_CONNECT:
        {
            int                         err;
            int                         errlen;
        
            sock_errlen = sizeof(sock_err);
            if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0)
            {
                err = errno;
            }
            
            if(err)
            {
                GlobusLXIOCloseFd(fd);
                result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
                    "globus_l_handle_write", errno);
            }
        }
        break;
      
      case GLOBUS_L_OPERATION_ACCEPT:
        {
            globus_sockaddr_t           addr;
            int                         addrlen;
            int                         new_fd;
    
            addrlen = sizeof(globus_sockaddr_t);
    
            do
            {
                new_fd = accept(fd, &addr, &addrlen);
            } while(new_fd < 0 && errno == EINTR);
            
            if(new_fd < 0)
            {
                result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
                    "globus_xio_system_open", errno);
            }

            *op_info->op.non_data.out_fd = new_fd;
        }
        break;
      
      case GLOBUS_L_OPERATION_WRITE:
        do
        {
            nbytes = write(
                fd,
                write_info->_op_single.buf,
                write_info->_op_single.bufsize);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            write_info->_op_single.buf = (char *)
                write_info->_op_single.buf + nbytes;
            write_info->_op_single.bufsize -= nbytes;
            write_info->_op_nbytes += nbytes;
        }
        break;
        
      case GLOBUS_L_OPERATION_WRITEV:
        do
        {
            nbytes = writev(
                fd, write_info->_op_iovec.iov, write_info->_op_iovec.iovc);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            write_info->_op_nbytes += nbytes;
            GlobusLAdjustIovec(
                write_info->_op_iovec.iov, write_info->_op_iovec.iovc, nbytes);
        }
        break;
        
      case GLOBUS_L_OPERATION_SEND:
        do
        {
            nbytes = send(
                fd,
                write_info->_op_single.buf,
                write_info->_op_single.bufsize,
                write_info->_op_single.ex.flags);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            write_info->_op_single.buf = (char *)
                write_info->_op_single.buf + nbytes;
            write_info->_op_single.bufsize -= nbytes;
            write_info->_op_nbytes += nbytes;
        }
        break;
        
      case GLOBUS_L_OPERATION_SENDTO:
        do
        {
            nbytes = sendto(
                fd,
                write_info->_op_single.buf,
                write_info->_op_single.bufsize,
                write_info->_op_single.ex.flags,
                write_info->_op_single.ex.addr,
                sizeof(globus_sockaddr_t));
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            write_info->_op_single.buf = (char *)
                write_info->_op_single.buf + nbytes;
            write_info->_op_single.bufsize -= nbytes;
            write_info->_op_nbytes += nbytes;
        }
        break;
        
      case GLOBUS_L_OPERATION_SENDMSG:
        do
        {
            nbytes = sendmsg(
                fd, write_info->_op_msg.msghdr, write_info->_op_msg.flags);
        } while(nbytes < 0 && errno == EINTR);
        
        if(nbytes > 0)
        {
            struct msghdr *             msghdr;
            
            write_info->_op_nbytes += nbytes;
            msghdr = write_info->_op_msg.msghdr;
            GlobusLAdjustIovec(msghdr->msg_iov, msghdr->msg_iovlen, nbytes);
        }
        break;
        
      case GLOBUS_L_OPERATION_READ:
      case GLOBUS_L_OPERATION_READV:
      case GLOBUS_L_OPERATION_RECV:
      case GLOBUS_L_OPERATION_RECVFROM:
      case GLOBUS_L_OPERATION_RECVMSG:
      default:
        globus_assert(0 && "Unexpected type for write operation");
        return GLOBUS_FALSE;
        break;
    }
    
    if(nbytes < 0)
    {
        result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_SYSTEM_ERROR(
            "globus_l_handle_write", errno);
    }
    
    if(write_info->_op_nbytes >= write_info->waitforbytes || 
        result != GLOBUS_SUCCESS)
    {
        handled_it = GLOBUS_TRUE;
        write_info->result = result;
        
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            globus_l_unregister_write(fd);
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
        
        result = globus_callback_register_oneshot(
            GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, write_info);
        globus_assert(result == GLOBUS_SUCCESS);
    }
    
    return handled_it;
}

static
void
globus_l_handle_canceled_read(
    int                                 fd)
{
    globus_l_operation_info_t *         read_info;
    globus_result_t                     result;
    
    read_info = globus_l_read_operations[fd];
    
    FD_CLR(fd, globus_l_canceled_reads);
    globus_mutex_lock(&globus_l_fdset_mutex);
    {
        globus_l_unregister_read(fd);
    }
    globus_mutex_unlock(&globus_l_fdset_mutex);
    
    read_info->result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
        "globus_l_handle_canceled_read");
    
    result = globus_callback_register_oneshot(
        GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, read_info);
    globus_assert(result == GLOBUS_SUCCESS);
}

static
void
globus_l_handle_canceled_write(
    int                                 fd)
{
    globus_l_operation_info_t *         write_info;
    globus_result_t                     result;
    
    write_info = globus_l_write_operations[fd];
    
    FD_CLR(fd, globus_l_canceled_writes);
    globus_mutex_lock(&globus_l_fdset_mutex);
    {
        globus_l_unregister_write(fd);
    }
    globus_mutex_unlock(&globus_l_fdset_mutex);
    
    write_info->result = GLOBUS_I_XIO_SYSTEM_CONSTRUCT_OPERATION_CANCELED(
        "globus_l_handle_canceled_write");
    
    result = globus_callback_register_oneshot(
        GLOBUS_NULL, GLOBUS_NULL, globus_l_kickout, write_info);
    globus_assert(result == GLOBUS_SUCCESS);
}

static 
void
globus_l_poll(
    void *                              user_args)
{
    globus_bool_t                       time_left_is_zero;
    globus_bool_t                       handled_something;

    do
    {
        globus_reltime_t                time_left;
        globus_bool_t                   time_left_is_infinity;
        int                             num;
        int                             nready;
        int                             select_errno;
        
        time_left_is_zero = GLOBUS_FALSE;
        time_left_is_infinity = GLOBUS_FALSE;
        
        globus_callback_get_timeout(&time_left);
        
        if(globus_reltime_cmp(&time_left, &globus_i_reltime_zero) == 0)
        {
            time_left_is_zero = GLOBUS_TRUE;
        }
        else if(globus_time_reltime_is_infinity(&time_left))
        {
            time_left_is_infinity = GLOBUS_TRUE;
        }   
        
        globus_mutex_lock(&globus_l_fdset_mutex);
        {
            num = (globus_l_highest_fd + 63) / 8;
            memcpy(globus_l_ready_reads, globus_l_read_fds, num);
            memcpy(globus_l_ready_writes, globus_l_write_fds, num);
        
            num = globus_l_highest_fd + 1;
            globus_l_select_active = GLOBUS_TRUE;
        }
        globus_mutex_unlock(&globus_l_fdset_mutex);
        
        nready = select(
            num,
            globus_l_ready_reads,
            globus_l_ready_writes,
            GLOBUS_NULL,
            (time_left_is_infinity ? GLOBUS_NULL : &time_left));
        select_errno = errno;
        
        globus_mutex_lock(&globus_l_cancel_mutex);
        {
            globus_mutex_lock(&globus_l_fdset_mutex);
            {
                globus_l_select_active = GLOBUS_FALSE;
            }
            globus_mutex_unlock(&globus_l_fdset_mutex);
            
            if(nready > 0)
            {
                int                     fd;
                
                fd = globus_l_wakeup_pipe[0];
                if(FD_ISSET(fd, globus_l_ready_reads))
                {
                    globus_l_handle_wakeup();
                    FD_CLR(fd, globus_l_ready_reads);
                    nready--;
                }
                
                for(fd = 0; nready; fd++)
                {
                    if(FD_ISSET(fd, globus_l_ready_reads))
                    {
                        nready--;
                        
                        if(!FD_ISSET(fd, globus_l_canceled_reads))
                        {
                            if(globus_l_handle_read(fd))
                            {
                                handled_something = GLOBUS_TRUE;
                            }
                        }
                        else
                        {
                            globus_l_handle_canceled_read(fd);
                            handled_something = GLOBUS_TRUE;
                        }
                    }
                    
                    if(FD_ISSET(fd, globus_l_ready_writes))
                    {
                        nready--;
                        
                        if(!FD_ISSET(fd, globus_l_canceled_writes))
                        {
                            if(globus_l_handle_write(fd))
                            {
                                handled_something = GLOBUS_TRUE;
                            }
                        }
                        else
                        {
                            globus_l_handle_canceled_write(fd);
                            handled_something = GLOBUS_TRUE;
                        }
                    }
                }
            }
            else if(nready == 0 || select_errno != EINTR)
            {
                time_left_is_zero = GLOBUS_TRUE;
            }
        }
        globus_mutex_unlock(&globus_l_cancel_mutex);
            
    } while(!handled_something && !time_left_is_zero && !globus_l_shutdown_called)
}
