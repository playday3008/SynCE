/* $Id$ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "synce.h"
#include "synce_log.h"

/** 
 * @defgroup SynceMisc Error and password handling
 * @ingroup SynceUtils
 * @brief Functions for reporting errors, and handling device passwords
 *
 * @{ 
 */ 

/** @brief Return an HRESULT using the given error code
 * 
 * This function maps the provided error code into 
 * an HRESULT, with a severity of SEVERITY_ERROR, and a
 * facility of FACILITY_WIN32.
 * 
 * @param[in] x error code
 * @return HRESULT for the given error code
 */ 
HRESULT
HRESULT_FROM_WIN32(DWORD x)
{
        return (HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);
}

char*
synce_strhresult(HRESULT hr)
{
	char *result_str = NULL;
	size_t result_len = 0;
	const char *sev_str = NULL;
	const char *code_str = NULL;

	if (SUCCEEDED(hr))
		sev_str = "Success: ";
	else
		sev_str = "Failure: ";

	if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
		code_str = synce_strerror(HRESULT_CODE(hr));
	else
		code_str = synce_strerror(hr);

	result_len = strlen(sev_str) + strlen(code_str) + 1;
	result_str = malloc(result_len);
	if (!result_str)
		return NULL;

	snprintf(result_str, result_len, "%s%s", sev_str, code_str);

	return result_str;
}

/** @brief Return string describing the error in the given HRESULT
 * 
 * This function extracts the error code from the given
 * HRESULT and returns a string describing the error code.
 * 
 * @param[in] hr HRESULT to describe
 * @return description of the error
 */ 
const char* synce_strerror_from_hresult(HRESULT hr)
{
  return synce_strerror(HRESULT_CODE(hr));
}

/** @brief Return string describing the given error code
 * 
 * This function returns a string describing the given
 * Windows error code.
 * 
 * @param[in] error error code to describe
 * @return description of the error
 */ 
const char* synce_strerror(DWORD error)
{
	char* result = NULL;
	
	switch (error)
	{
		case E_ABORT:                                    result = "The operation was aborted because of an unspecified error"; break;
		case E_ACCESSDENIED:                             result = "A general access-denied error"; break;
		case E_FAIL:                                     result = "An unspecified failure has occurred"; 	break;
		case E_HANDLE:                                   result = "An invalid handle was used"; break;
		case E_INVALIDARG:                               result = "One or more arguments are invalid."; break;

		case E_NOTIMPL:                                  result = "The method is not implemented"; break;
		case E_OUTOFMEMORY:                              result = "The method failed to allocate necessary memory"; break;
		case E_PENDING:                                  result = "The data necessary to complete the operation is not yet available"; break;
		case E_POINTER:                                  result = "An invalid pointer was used"; break;
		case E_UNEXPECTED:                               result = "A catastrophic failure has occurred"; break;

		case E_NOINTERFACE:                              result = "No such interface supported."; break;

		case ERROR_SUCCESS:                              result = "The operation completed successfully."; break;
		case ERROR_INVALID_FUNCTION:                     result = "Incorrect function."; break;
		case ERROR_FILE_NOT_FOUND:                       result = "The system cannot find the file specified."; break;
		case ERROR_PATH_NOT_FOUND:                       result = "The system cannot find the path specified."; break;
		case ERROR_TOO_MANY_OPEN_FILES:                  result = "The system cannot open the file."; break;
		case ERROR_ACCESS_DENIED:                        result = "Access is denied."; break;
		case ERROR_INVALID_HANDLE:                       result = "The handle is invalid."; break;
		case ERROR_ARENA_TRASHED:                        result = "The storage control blocks were destroyed."; break;
		case ERROR_NOT_ENOUGH_MEMORY:                    result = "Not enough storage is available to process this command."; break;
		case ERROR_INVALID_BLOCK:                        result = "The storage control block address is invalid."; break;
		case ERROR_BAD_ENVIRONMENT:                      result = "The environment is incorrect."; break;
		case ERROR_BAD_FORMAT:                           result = "An attempt was made to load a program with an incorrect format."; break;
		case ERROR_INVALID_ACCESS:                       result = "The access code is invalid."; break;
		case ERROR_INVALID_DATA:                         result = "The data is invalid."; break;
		case ERROR_OUTOFMEMORY:                          result = "Not enough storage is available to complete this operation."; break;
		case ERROR_INVALID_DRIVE:                        result = "The system cannot find the drive specified."; break;
		case ERROR_CURRENT_DIRECTORY:                    result = "The directory cannot be removed."; break;
		case ERROR_NOT_SAME_DEVICE:                      result = "The system cannot move the file to a different disk drive."; break;
		case ERROR_NO_MORE_FILES:                        result = "There are no more files."; break;
		case ERROR_WRITE_PROTECT:                        result = "The media is write protected."; break;
		case ERROR_BAD_UNIT:                             result = "The system cannot find the specified device."; break;
		case ERROR_NOT_READY:                            result = "The device is not ready."; break;
		case ERROR_BAD_COMMAND:                          result = "The device does not recognize the command."; break;
		case ERROR_CRC:                                  result = "Data error (cyclic redundancy check)."; break;
		case ERROR_BAD_LENGTH:                           result = "The program issued a command but the command length is incorrect."; break;
		case ERROR_SEEK:                                 result = "The drive cannot locate a specific area or track on the disk."; break;
		case ERROR_NOT_DOS_DISK:                         result = "The specified disk or diskette cannot be accessed."; break;
		case ERROR_SECTOR_NOT_FOUND:                     result = "The drive cannot find the sector requested."; break;
		case ERROR_OUT_OF_PAPER:                         result = "The printer is out of paper."; break;
		case ERROR_WRITE_FAULT:                          result = "The system cannot write to the specified device."; break;
		case ERROR_READ_FAULT:                           result = "The system cannot read from the specified device."; break;
		case ERROR_GEN_FAILURE:                          result = "A device attached to the system is not functioning."; break;
		case ERROR_SHARING_VIOLATION:                    result = "The process cannot access the file because it is being used by another process."; break;
		case ERROR_LOCK_VIOLATION:                       result = "The process cannot access the file because another process has locked a portion of the file."; break;
		case ERROR_WRONG_DISK:                           result = "The wrong diskette is in the drive. Insert %2 (Volume Serial Number: %3) into drive %1."; break;
		case ERROR_SHARING_BUFFER_EXCEEDED:              result = "Too many files opened for sharing."; break;
		case ERROR_HANDLE_EOF:                           result = "Reached the end of the file."; break;
		case ERROR_HANDLE_DISK_FULL:                     result = "The disk is full."; break;
		case ERROR_NOT_SUPPORTED:                        result = "The network request is not supported."; break;
		case ERROR_REM_NOT_LIST:                         result = "The remote computer is not available."; break;
		case ERROR_DUP_NAME:                             result = "A duplicate name exists on the network."; break;
		case ERROR_BAD_NETPATH:                          result = "The network path was not found."; break;
		case ERROR_NETWORK_BUSY:                         result = "The network is busy."; break;
		case ERROR_DEV_NOT_EXIST:                        result = "The specified network resource or device is no longer available."; break;
		case ERROR_TOO_MANY_CMDS:                        result = "The network BIOS command limit has been reached."; break;
		case ERROR_ADAP_HDW_ERR:                         result = "A network adapter hardware error occurred."; break;
		case ERROR_BAD_NET_RESP:                         result = "The specified server cannot perform the requested operation."; break;
		case ERROR_UNEXP_NET_ERR:                        result = "An unexpected network error occurred."; break;
		case ERROR_BAD_REM_ADAP:                         result = "The remote adapter is not compatible."; break;
		case ERROR_PRINTQ_FULL:                          result = "The printer queue is full."; break;
		case ERROR_NO_SPOOL_SPACE:                       result = "Space to store the file waiting to be printed is not available on the server."; break;
		case ERROR_PRINT_CANCELLED:                      result = "Your file waiting to be printed was deleted."; break;
		case ERROR_NETNAME_DELETED:                      result = "The specified network name is no longer available."; break;
		case ERROR_NETWORK_ACCESS_DENIED:                result = "Network access is denied."; break;
		case ERROR_BAD_DEV_TYPE:                         result = "The network resource type is not correct."; break;
		case ERROR_BAD_NET_NAME:                         result = "The network name cannot be found."; break;
		case ERROR_TOO_MANY_NAMES:                       result = "The name limit for the local computer network adapter card was exceeded."; break;
		case ERROR_TOO_MANY_SESS:                        result = "The network BIOS session limit was exceeded."; break;
		case ERROR_SHARING_PAUSED:                       result = "The remote server has been paused or is in the process of being started."; break;
		case ERROR_REQ_NOT_ACCEP:                        result = "No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept."; break;
		case ERROR_REDIR_PAUSED:                         result = "The specified printer or disk device has been paused."; break;
		case ERROR_FILE_EXISTS:                          result = "The file exists."; break;
		case ERROR_CANNOT_MAKE:                          result = "The directory or file cannot be created."; break;
		case ERROR_FAIL_I24:                             result = "Fail oninterrupt 24 handler."; break;
		case ERROR_OUT_OF_STRUCTURES:                    result = "Storage to process this request is not available."; break;
		case ERROR_ALREADY_ASSIGNED:                     result = "The local device name is already in use."; break;
		case ERROR_INVALID_PASSWORD:                     result = "The specified network password is not correct."; break;
		case ERROR_INVALID_PARAMETER:                    result = "The parameter is incorrect."; break;
		case ERROR_NET_WRITE_FAULT:                      result = "A write fault occurred on the network."; break;
		case ERROR_NO_PROC_SLOTS:                        result = "The system cannot start another process at this time."; break;
		case ERROR_TOO_MANY_SEMAPHORES:                  result = "Cannot create another system semaphore."; break;
		case ERROR_EXCL_SEM_ALREADY_OWNED:               result = "The exclusive semaphore is owned by another process."; break;
		case ERROR_SEM_IS_SET:                           result = "The semaphore is set and cannot be closed."; break;
		case ERROR_TOO_MANY_SEM_REQUESTS:                result = "The semaphore cannot be set again."; break;
		case ERROR_INVALID_AT_INTERRUPT_TIME:            result = "Cannot request exclusive semaphores at interrupt time."; break;
		case ERROR_SEM_OWNER_DIED:                       result = "The previous ownership of this semaphore has ended."; break;
		case ERROR_SEM_USER_LIMIT:                       result = "Insert the diskette for drive %1."; break;
		case ERROR_DISK_CHANGE:                          result = "The program stopped because an alternate diskette was not inserted."; break;
		case ERROR_DRIVE_LOCKED:                         result = "The disk is in use or locked by another process."; break;
		case ERROR_BROKEN_PIPE:                          result = "The pipe has been ended."; break;
		case ERROR_OPEN_FAILED:                          result = "The system cannot open the device or file specified."; break;
		case ERROR_BUFFER_OVERFLOW:                      result = "The file name is too long."; break;
		case ERROR_DISK_FULL:                            result = "There is not enough space on the disk."; break;
		case ERROR_NO_MORE_SEARCH_HANDLES:               result = "No more internal file identifiers available."; break;
		case ERROR_INVALID_TARGET_HANDLE:                result = "The target internal file identifier is incorrect."; break;
		case ERROR_INVALID_CATEGORY:                     result = "The IOCTL call made by the application program is not correct."; break;
		case ERROR_INVALID_VERIFY_SWITCH:                result = "The verify-on-write switch parameter value is not correct."; break;
		case ERROR_BAD_DRIVER_LEVEL:                     result = "The system does not support the command requested."; break;
		case ERROR_CALL_NOT_IMPLEMENTED:                 result = "This function is not valid on this platform."; break;
		case ERROR_SEM_TIMEOUT:                          result = "The semaphore time-out period has expired."; break;
		case ERROR_INSUFFICIENT_BUFFER:                  result = "The data area passed to a system call is too small."; break;
		case ERROR_INVALID_NAME:                         result = "The file name, directory name, or volume label syntax is incorrect."; break;
		case ERROR_INVALID_LEVEL:                        result = "The system call level is not correct."; break;
		case ERROR_NO_VOLUME_LABEL:                      result = "The disk has no volume label."; break;
		case ERROR_MOD_NOT_FOUND:                        result = "The specified module could not be found."; break;
		case ERROR_PROC_NOT_FOUND:                       result = "The specified procedure could not be found."; break;
		case ERROR_WAIT_NO_CHILDREN:                     result = "There are no child processes to wait for."; break;
		case ERROR_CHILD_NOT_COMPLETE:                   result = "The %1 application cannot be run in Windows NT mode."; break;
		case ERROR_DIRECT_ACCESS_HANDLE:                 result = "Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O."; break;
		case ERROR_NEGATIVE_SEEK:                        result = "An attempt was made to move the file pointer before the beginning of the file."; break;
		case ERROR_SEEK_ON_DEVICE:                       result = "The file pointer cannot be set on the specified device or file."; break;
		case ERROR_IS_JOIN_TARGET:                       result = "A JOIN or SUBST command cannot be used for a drive that contains previously joined drives."; break;
		case ERROR_IS_JOINED:                            result = "An attempt was made to use a JOIN or SUBST command on a drive that has already been joined."; break;
		case ERROR_IS_SUBSTED:                           result = "An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted."; break;
		case ERROR_NOT_JOINED:                           result = "The system tried to delete the JOIN of a drive that is not joined."; break;
		case ERROR_NOT_SUBSTED:                          result = "The system tried to delete the substitution of a drive that is not substituted."; break;
		case ERROR_JOIN_TO_JOIN:                         result = "The system tried to join a drive to a directory on a joined drive."; break;
		case ERROR_SUBST_TO_SUBST:                       result = "The system tried to substitute a drive to a directory on a substituted drive."; break;
		case ERROR_JOIN_TO_SUBST:                        result = "The system tried to join a drive to a directory on a substituted drive."; break;
		case ERROR_SUBST_TO_JOIN:                        result = "The system tried to SUBST a drive to a directory on a joined drive."; break;
		case ERROR_BUSY_DRIVE:                           result = "The system cannot perform a JOIN or SUBST at this time."; break;
		case ERROR_SAME_DRIVE:                           result = "The system cannot join or substitute a drive to or for a directory on the same drive."; break;
		case ERROR_DIR_NOT_ROOT:                         result = "The directory is not a subdirectory of the root directory."; break;
		case ERROR_DIR_NOT_EMPTY:                        result = "The directory is not empty."; break;
		case ERROR_IS_SUBST_PATH:                        result = "The path specified is being used in a substitute."; break;
		case ERROR_IS_JOIN_PATH:                         result = "Not enough resources are available to process this command."; break;
		case ERROR_PATH_BUSY:                            result = "The path specified cannot be used at this time."; break;
		case ERROR_IS_SUBST_TARGET:                      result = "An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute."; break;
		case ERROR_SYSTEM_TRACE:                         result = "System trace information was not specified in your Config.sys file, or tracing is disallowed."; break;
		case ERROR_INVALID_EVENT_COUNT:                  result = "The number of specified semaphore events for DosMuxSemWait is not correct."; break;
		case ERROR_TOO_MANY_MUXWAITERS:                  result = "DosMuxSemWait did not execute; too many semaphores are already set."; break;
		case ERROR_INVALID_LIST_FORMAT:                  result = "The DosMuxSemWait list is not correct."; break;
		case ERROR_LABEL_TOO_LONG:                       result = "The volume label you entered exceeds the label character limit of the target file system."; break;
		case ERROR_TOO_MANY_TCBS:                        result = "Cannot create another thread."; break;
		case ERROR_SIGNAL_REFUSED:                       result = "The recipient process has refused the signal."; break;
		case ERROR_DISCARDED:                            result = "The segment is already discarded and cannot be locked."; break;
		case ERROR_NOT_LOCKED:                           result = "The segment is already unlocked."; break;
		case ERROR_BAD_THREADID_ADDR:                    result = "The address for the thread identifier is not correct."; break;
		case ERROR_BAD_ARGUMENTS:                        result = "The argument string passed to DosExecPgm is not correct."; break;
		case ERROR_BAD_PATHNAME:                         result = "The specified path is invalid."; break;
		case ERROR_SIGNAL_PENDING:                       result = "A signal is already pending."; break;
		case ERROR_MAX_THRDS_REACHED:                    result = "No more threads can be created in the system."; break;
		case ERROR_LOCK_FAILED:                          result = "Unable to lock a region of a file."; break;
		case ERROR_BUSY:                                 result = "The requested resource is in use."; break;
		case ERROR_CANCEL_VIOLATION:                     result = "A lock request was not outstanding for the supplied cancel region."; break;
		case ERROR_ATOMIC_LOCKS_NOT_SUPPORTED:           result = "The file system does not support atomic changes to the lock type."; break;
		case ERROR_INVALID_SEGMENT_NUMBER:               result = "The system detected a segment number that was not correct."; break;
		case ERROR_INVALID_ORDINAL:                      result = "The operating system cannot run %1."; break;
		case ERROR_ALREADY_EXISTS:                       result = "Cannot create a file when that file already exists."; break;
		case ERROR_INVALID_FLAG_NUMBER:                  result = "The flag passed is not correct."; break;
		case ERROR_SEM_NOT_FOUND:                        result = "The specified system semaphore name was not found."; break;
		case ERROR_INVALID_STARTING_CODESEG:             result = "The operating system cannot run %1."; break;
		case ERROR_INVALID_STACKSEG:                     result = "The operating system cannot run %1."; break;
		case ERROR_INVALID_MODULETYPE:                   result = "The operating system cannot run %1."; break;
		case ERROR_INVALID_EXE_SIGNATURE:                result = "Cannot run %1 in Windows NT mode."; break;
		case ERROR_EXE_MARKED_INVALID:                   result = "The operating system cannot run %1."; break;
		case ERROR_BAD_EXE_FORMAT:                       result = "Is not a valid application."; break;
		case ERROR_ITERATED_DATA_EXCEEDS_64k:            result = "The operating system cannot run %1."; break;
		case ERROR_INVALID_MINALLOCSIZE:                 result = "The operating system cannot run %1."; break;
		case ERROR_DYNLINK_FROM_INVALID_RING:            result = "The operating system cannot run this application program."; break;
		case ERROR_IOPL_NOT_ENABLED:                     result = "The operating system is not presently configured to run this application."; break;
		case ERROR_INVALID_SEGDPL:                       result = "The operating system cannot run %1."; break;
		case ERROR_AUTODATASEG_EXCEEDS_64k:              result = "The operating system cannot run this application program."; break;
		case ERROR_RING2SEG_MUST_BE_MOVABLE:             result = "The code segment cannot be greater than or equal to 64 KB."; break;
		case ERROR_RELOC_CHAIN_XEEDS_SEGLIM:             result = "The operating system cannot run %1."; break;
		case ERROR_INFLOOP_IN_RELOC_CHAIN:               result = "The operating system cannot run %1."; break;
		case ERROR_ENVVAR_NOT_FOUND:                     result = "The system could not find the environment option that was entered."; break;
		case ERROR_NO_SIGNAL_SENT:                       result = "No process in the command subtree has a signal handler."; break;
		case ERROR_FILENAME_EXCED_RANGE:                 result = "The file name or extension is too long."; break;
		case ERROR_RING2_STACK_IN_USE:                   result = "The ring 2 stack is in use."; break;
		case ERROR_META_EXPANSION_TOO_LONG:              result = "The global file name characters, * or ?, are entered incorrectly or too many global file name characters are specified."; break;
		case ERROR_INVALID_SIGNAL_NUMBER:                result = "The signal being posted is not correct."; break;
		case ERROR_THREAD_1_INACTIVE:                    result = "The signal handler cannot be set."; break;
		case ERROR_LOCKED:                               result = "The segment is locked and cannot be reallocated."; break;
		case ERROR_TOO_MANY_MODULES:                     result = "Too many dynamic-link modules are attached to this program or dynamic-link module."; break;
		case ERROR_NESTING_NOT_ALLOWED:                  result = "Cannot nest calls to the LoadModule function."; break;
		case ERROR_EXE_MACHINE_TYPE_MISMATCH:            result = "The image file %1 is valid, but is for a machine type other than the current machine."; break;
		case ERROR_BAD_PIPE:                             result = "The pipe state is invalid."; break;
		case ERROR_PIPE_BUSY:                            result = "All pipe instances are busy."; break;
		case ERROR_NO_DATA:                              result = "The pipe is being closed."; break;
		case ERROR_PIPE_NOT_CONNECTED:                   result = "No process is on the other end of the pipe."; break;
		case ERROR_MORE_DATA:                            result = "More data is available."; break;
		case ERROR_VC_DISCONNECTED:                      result = "The session was canceled."; break;
		case ERROR_INVALID_EA_NAME:                      result = "The specified extended attribute name was invalid."; break;
		case ERROR_EA_LIST_INCONSISTENT:                 result = "The extended attributes are inconsistent."; break;
		case ERROR_NO_MORE_ITEMS:                        result = "No more data is available."; break;
		case ERROR_CANNOT_COPY:                          result = "The copy functions cannot be used."; break;
		case ERROR_DIRECTORY:                            result = "The directory name is invalid."; break;
		case ERROR_EAS_DIDNT_FIT:                        result = "The extended attributes did not fit in the buffer."; break;
		case ERROR_EA_FILE_CORRUPT:                      result = "The extended attribute file on the mounted file system is corrupt."; break;
		case ERROR_EA_TABLE_FULL:                        result = "The extended attribute table file is full."; break;
		case ERROR_INVALID_EA_HANDLE:                    result = "The specified extended attribute handle is invalid."; break;
		case ERROR_EAS_NOT_SUPPORTED:                    result = "The mounted file system does not support extended attributes."; break;
		case ERROR_NOT_OWNER:                            result = "Attempt to release mutex not owned by caller."; break;
		case ERROR_TOO_MANY_POSTS:                       result = "Too many posts were made to a semaphore."; break;
		case ERROR_PARTIAL_COPY:                         result = "Only part of a ReadProcessMemory or WriteProcessMemory request was completed."; break;
		case ERROR_MR_MID_NOT_FOUND:                     result = "The system cannot find message text for message number 0x%1 in the message file for %2."; break;
		case ERROR_INVALID_ADDRESS:                      result = "Attempt to access invalid address."; break;
		case ERROR_ARITHMETIC_OVERFLOW:                  result = "Arithmetic result exceeded 32 bits."; break;
		case ERROR_PIPE_CONNECTED:                       result = "There is a process on other end of the pipe."; break;
		case ERROR_PIPE_LISTENING:                       result = "Waiting for a process to open the other end of the pipe."; break;
		case ERROR_EA_ACCESS_DENIED:                     result = "Access to the extended attribute was denied."; break;
		case ERROR_OPERATION_ABORTED:                    result = "The I/O operation has been aborted because of either a thread exit or an application request."; break;
		case ERROR_IO_INCOMPLETE:                        result = "Overlapped I/O event is not in a signaled state."; break;
		case ERROR_IO_PENDING:                           result = "Overlapped I/O operation is in progress."; break;
		case ERROR_NOACCESS:                             result = "Invalid access to memory location."; break;
		case ERROR_SWAPERROR:                            result = "Error performing inpage operation."; break;
		case ERROR_STACK_OVERFLOW:                       result = "Recursion too deep; the stack overflowed."; break;
		case ERROR_INVALID_MESSAGE:                      result = "The window cannot act on the sent message."; break;
		case ERROR_CAN_NOT_COMPLETE:                     result = "Cannot complete this function."; break;
		case ERROR_INVALID_FLAGS:                        result = "Invalid flags."; break;
		case ERROR_UNRECOGNIZED_VOLUME:                  result = "The volume does not contain a recognized file system. Verify that all required file system drivers are loaded and that the volume is not corrupted."; break;
		case ERROR_FILE_INVALID:                         result = "The volume for a file has been externally altered so that the opened file is no longer valid."; break;
		case ERROR_FULLSCREEN_MODE:                      result = "The requested operation cannot be performed in full-screen mode."; break;
		case ERROR_NO_TOKEN:                             result = "An attempt was made to reference a token that does not exist."; break;
		case ERROR_BADDB:                                result = "The configuration registry database is corrupt."; break;
		case ERROR_BADKEY:                               result = "The configuration registry key is invalid."; break;
		case ERROR_CANTOPEN:                             result = "The configuration registry key could not be opened."; break;
		case ERROR_CANTREAD:                             result = "The configuration registry key could not be read."; break;
		case ERROR_CANTWRITE:                            result = "The configuration registry key could not be written."; break;
		case ERROR_REGISTRY_RECOVERED:                   result = "One of the files in the registry database had to be recovered by use of a log or alternate copy. The recovery was successful."; break;
		case ERROR_REGISTRY_CORRUPT:                     result = "The registry is corrupted. The structure of one of the files that contains registry data is corrupted, or the system's image of the file in memory is corrupted, or the file could not be recovered because the alternate copy or log was absent or corrupted."; break;
		case ERROR_REGISTRY_IO_FAILED:                   result = "An I/O operation initiated by the registry failed unrecoverably. The registry could not read in, or write out, or flush, one of the files that contain the system's image of the registry."; break;
		case ERROR_NOT_REGISTRY_FILE:                    result = "The system has attempted to load or restore a file into the registry, but the specified file is not in a registry file format."; break;
		case ERROR_KEY_DELETED:                          result = "Illegal operation attempted on a registry key that has been marked for deletion."; break;
		case ERROR_NO_LOG_SPACE:                         result = "System could not allocate the required space in a registry log."; break;
		case ERROR_KEY_HAS_CHILDREN:                     result = "Cannot create a symbolic link in a registry key that already has subkeys or values."; break;
		case ERROR_CHILD_MUST_BE_VOLATILE:               result = "Cannot create a stable subkey under a volatile parent key."; break;
		case ERROR_NOTIFY_ENUM_DIR:                      result = "A notify change request is being completed and the information is not being returned in the caller's buffer. The caller now needs to enumerate the files to find the changes."; break;
		case ERROR_DEPENDENT_SERVICES_RUNNING:           result = "A stop control has been sent to a service that other running services are dependent on."; break;
		case ERROR_INVALID_SERVICE_CONTROL:              result = "The requested control is not valid for this service."; break;
		case ERROR_SERVICE_REQUEST_TIMEOUT:              result = "The service did not respond to the start or control request in a timely fashion."; break;
		case ERROR_SERVICE_NO_THREAD:                    result = "A thread could not be created for the service."; break;
		case ERROR_SERVICE_DATABASE_LOCKED:              result = "The service database is locked."; break;
		case ERROR_SERVICE_ALREADY_RUNNING:              result = "An instance of the service is already running."; break;
		case ERROR_INVALID_SERVICE_ACCOUNT:              result = "The account name is invalid or does not exist."; break;
		case ERROR_SERVICE_DISABLED:                     result = "The specified service is disabled and cannot be started."; break;
		case ERROR_CIRCULAR_DEPENDENCY:                  result = "Circular service dependency was specified."; break;
		case ERROR_SERVICE_DOES_NOT_EXIST:               result = "The specified service does not exist as an installed service."; break;
		case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:           result = "The service cannot accept control messages at this time."; break;
		case ERROR_SERVICE_NOT_ACTIVE:                   result = "The service has not been started."; break;
		case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:    result = "The service process could not connect to the service controller."; break;
		case ERROR_EXCEPTION_IN_SERVICE:                 result = "An exception occurred in the service when handling the control request."; break;
		case ERROR_DATABASE_DOES_NOT_EXIST:              result = "The database specified does not exist."; break;
		case ERROR_SERVICE_SPECIFIC_ERROR:               result = "The service has returned a service-specific error code."; break;
		case ERROR_PROCESS_ABORTED:                      result = "The process terminated unexpectedly."; break;
		case ERROR_SERVICE_DEPENDENCY_FAIL:              result = "The dependency service or group failed to start."; break;
		case ERROR_SERVICE_LOGON_FAILED:                 result = "The service did not start due to a logon failure."; break;
		case ERROR_SERVICE_START_HANG:                   result = "After starting, the service hung in a start-pending state."; break;
		case ERROR_INVALID_SERVICE_LOCK:                 result = "The specified service database lock is invalid."; break;
		case ERROR_SERVICE_MARKED_FOR_DELETE:            result = "The specified service has been marked for deletion."; break;
		case ERROR_SERVICE_EXISTS:                       result = "The specified service already exists."; break;
		case ERROR_ALREADY_RUNNING_LKG:                  result = "The system is currently running with the last-known-good configuration."; break;
		case ERROR_SERVICE_DEPENDENCY_DELETED:           result = "The dependency service does not exist or has been marked for deletion."; break;
		case ERROR_BOOT_ALREADY_ACCEPTED:                result = "The current boot has already been accepted for use as the last-known-good control set."; break;
		case ERROR_SERVICE_NEVER_STARTED:                result = "No attempts to start the service have been made since the last boot."; break;
		case ERROR_DUPLICATE_SERVICE_NAME:               result = "The name is already in use as either a service name or a service display name."; break;
		case ERROR_DIFFERENT_SERVICE_ACCOUNT:            result = "The account specified for this service is different from the account specified for other services running in the same process."; break;
		case ERROR_END_OF_MEDIA:                         result = "The physical end of the tape has been reached."; break;
		case ERROR_FILEMARK_DETECTED:                    result = "A tape access reached a filemark."; break;
		case ERROR_BEGINNING_OF_MEDIA:                   result = "The beginning of the tape or partition was encountered."; break;
		case ERROR_SETMARK_DETECTED:                     result = "A tape access reached the end of a set of files."; break;
		case ERROR_NO_DATA_DETECTED:                     result = "No more data is on the tape."; break;
		case ERROR_PARTITION_FAILURE:                    result = "Tape could not be partitioned."; break;
		case ERROR_INVALID_BLOCK_LENGTH:                 result = "When accessing a new tape of a multivolume partition, the current block size is incorrect."; break;
		case ERROR_DEVICE_NOT_PARTITIONED:               result = "Tape partition information could not be found when loading a tape."; break;
		case ERROR_UNABLE_TO_LOCK_MEDIA:                 result = "Unable to lock the media eject mechanism."; break;
		case ERROR_UNABLE_TO_UNLOAD_MEDIA:               result = "Unable to unload the media."; break;
		case ERROR_MEDIA_CHANGED:                        result = "The media in the drive may have changed."; break;
		case ERROR_BUS_RESET:                            result = "The I/O bus was reset."; break;
		case ERROR_NO_MEDIA_IN_DRIVE:                    result = "No media in drive."; break;
		case ERROR_NO_UNICODE_TRANSLATION:               result = "No mapping for the Unicode character exists in the target multibyte code page."; break;
		case ERROR_DLL_INIT_FAILED:                      result = "A dynamic link library (DLL) initialization routine failed."; break;
		case ERROR_SHUTDOWN_IN_PROGRESS:                 result = "A system shutdown is in progress."; break;
		case ERROR_NO_SHUTDOWN_IN_PROGRESS:              result = "Unable to abort the system shutdown because no shutdown was in progress."; break;
		case ERROR_IO_DEVICE:                            result = "The request could not be performed because of an I/O device error."; break;
		case ERROR_SERIAL_NO_DEVICE:                     result = "No serial device was successfully initialized. The serial driver will unload."; break;
		case ERROR_IRQ_BUSY:                             result = "Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened."; break;
		case ERROR_MORE_WRITES:                          result = "A serial I/O operation was completed by another write to the serial port. The IOCTL_SERIAL_XOFF_COUNTER reached zero.)"; break;
		case ERROR_COUNTER_TIMEOUT:                      result = "A serial I/O operation completed because the time-out period expired. In other words, the IOCTL_SERIAL_XOFF_COUNTER did not reach zero."; break;
		case ERROR_FLOPPY_ID_MARK_NOT_FOUND:             result = "No identifier address mark was found on the floppy disk."; break;
		case ERROR_FLOPPY_WRONG_CYLINDER:                result = "Mismatch between the floppy disk sector identifier field and the floppy disk controller track address."; break;
		case ERROR_FLOPPY_UNKNOWN_ERROR:                 result = "The floppy disk controller reported an error that is not recognized by the floppy disk driver."; break;
		case ERROR_FLOPPY_BAD_REGISTERS:                 result = "The floppy disk controller returned inconsistent results in its registers."; break;
		case ERROR_DISK_RECALIBRATE_FAILED:              result = "While accessing the hard disk, a recalibrate operation failed, even after retries."; break;
		case ERROR_DISK_OPERATION_FAILED:                result = "While accessing the hard disk, a disk operation failed even after retries."; break;
		case ERROR_DISK_RESET_FAILED:                    result = "While accessing the hard disk, a disk controller reset was needed, but even that failed."; break;
		case ERROR_EOM_OVERFLOW:                         result = "Physical end of tape encountered."; break;
		case ERROR_NOT_ENOUGH_SERVER_MEMORY:             result = "Not enough server storage is available to process this command."; break;
		case ERROR_POSSIBLE_DEADLOCK:                    result = "A potential deadlock condition has been detected."; break;
		case ERROR_MAPPED_ALIGNMENT:                     result = "The base address or the file offset specified does not have the proper alignment."; break;
		case ERROR_SET_POWER_STATE_VETOED:               result = "An attempt to change the system power state was vetoed by another application or driver."; break;
		case ERROR_SET_POWER_STATE_FAILED:               result = "The basic input/output system (BIOS) failed an attempt to change the system power state."; break;
		case ERROR_TOO_MANY_LINKS:                       result = "An attempt was made to create more links on a file than the file system supports."; break;
		case ERROR_OLD_WIN_VERSION:                      result = "The specified program requires a newer version of Windows."; break;
		case ERROR_APP_WRONG_OS:                         result = "The specified program is not a Windows or MS-DOS program."; break;
		case ERROR_SINGLE_INSTANCE_APP:                  result = "Cannot start more than one instance of the specified program."; break;
		case ERROR_RMODE_APP:                            result = "The specified program was written for an earlier version of Windows."; break;
		case ERROR_INVALID_DLL:                          result = "One of the library files needed to run this application is damaged."; break;
		case ERROR_NO_ASSOCIATION:                       result = "No application is associated with the specified file for this operation."; break;
		case ERROR_DDE_FAIL:                             result = "An error occurred in sending the command to the application."; break;
		case ERROR_DLL_NOT_FOUND:                        result = "One of the library files needed to run this application cannot be found."; break;
		case ERROR_BAD_DEVICE:                           result = "The specified device name is invalid."; break;
		case ERROR_CONNECTION_UNAVAIL:                   result = "The device is not currently connected but it is a remembered connection."; break;
		case ERROR_DEVICE_ALREADY_REMEMBERED:            result = "An attempt was made to remember a device that had previously been remembered."; break;
		case ERROR_NO_NET_OR_BAD_PATH:                   result = "No network provider accepted the given network path."; break;
		case ERROR_BAD_PROVIDER:                         result = "The specified network provider name is invalid."; break;
		case ERROR_CANNOT_OPEN_PROFILE:                  result = "Unable to open the network connection profile."; break;
		case ERROR_BAD_PROFILE:                          result = "The network connection profile is corrupt."; break;
		case ERROR_NOT_CONTAINER:                        result = "Cannot enumerate a noncontainer."; break;
		case ERROR_EXTENDED_ERROR:                       result = "An extended error has occurred."; break;
		case ERROR_INVALID_GROUPNAME:                    result = "The format of the specified group name is invalid."; break;
		case ERROR_INVALID_COMPUTERNAME:                 result = "The format of the specified computer name is invalid."; break;
		case ERROR_INVALID_EVENTNAME:                    result = "The format of the specified event name is invalid."; break;
		case ERROR_INVALID_DOMAINNAME:                   result = "The format of the specified domain name is invalid."; break;
		case ERROR_INVALID_SERVICENAME:                  result = "The format of the specified service name is invalid."; break;
		case ERROR_INVALID_NETNAME:                      result = "The format of the specified network name is invalid."; break;
		case ERROR_INVALID_SHARENAME:                    result = "The format of the specified share name is invalid."; break;
		case ERROR_INVALID_PASSWORDNAME:                 result = "The format of the specified password is invalid."; break;
		case ERROR_INVALID_MESSAGENAME:                  result = "The format of the specified message name is invalid."; break;
		case ERROR_INVALID_MESSAGEDEST:                  result = "The format of the specified message destination is invalid."; break;
		case ERROR_SESSION_CREDENTIAL_CONFLICT:          result = "The credentials supplied conflict with an existing set of credentials."; break;
		case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED:        result = "An attempt was made to establish a session to a network server, but there are already too many sessions established to that server."; break;
		case ERROR_DUP_DOMAINNAME:                       result = "The workgroup or domain name is already in use by another computer on the network."; break;
		case ERROR_NO_NETWORK:                           result = "The network is not present or not started."; break;
		case ERROR_CANCELLED:                            result = "The operation was canceled by the user."; break;
		case ERROR_USER_MAPPED_FILE:                     result = "The requested operation cannot be performed on a file with a user-mapped section open."; break;
		case ERROR_CONNECTION_REFUSED:                   result = "The remote system refused the network connection."; break;
		case ERROR_GRACEFUL_DISCONNECT:                  result = "The network connection was gracefully closed."; break;
		case ERROR_ADDRESS_ALREADY_ASSOCIATED:           result = "The network transport endpoint already has an address associated with it."; break;
		case ERROR_ADDRESS_NOT_ASSOCIATED:               result = "An address has not yet been associated with the network endpoint."; break;
		case ERROR_CONNECTION_INVALID:                   result = "An operation was attempted on a nonexistent network connection."; break;
		case ERROR_CONNECTION_ACTIVE:                    result = "An invalid operation was attempted on an active network connection."; break;
		case ERROR_NETWORK_UNREACHABLE:                  result = "The remote network is not reachable by the transport."; break;
		case ERROR_HOST_UNREACHABLE:                     result = "The remote system is not reachable by the transport."; break;
		case ERROR_PROTOCOL_UNREACHABLE:                 result = "The remote system does not support the transport protocol."; break;
		case ERROR_PORT_UNREACHABLE:                     result = "No service is operating at the destination network endpoint on the remote system."; break;
		case ERROR_REQUEST_ABORTED:                      result = "The request was aborted."; break;
		case ERROR_CONNECTION_ABORTED:                   result = "The network connection was aborted by the local system."; break;
		case ERROR_RETRY:                                result = "The operation could not be completed. A retry should be performed."; break;
		case ERROR_CONNECTION_COUNT_LIMIT:               result = "A connection to the server could not be made because the limit on the number of concurrent connections for this account has been reached."; break;
		case ERROR_LOGIN_TIME_RESTRICTION:               result = "Attempting to log in during an unauthorized time of day for this account."; break;
		case ERROR_LOGIN_WKSTA_RESTRICTION:              result = "The account is not authorized to log in from this station."; break;
		case ERROR_INCORRECT_ADDRESS:                    result = "The network address could not be used for the operation requested."; break;
		case ERROR_ALREADY_REGISTERED:                   result = "The service is already registered."; break;
		case ERROR_SERVICE_NOT_FOUND:                    result = "The specified service does not exist."; break;
		case ERROR_NOT_AUTHENTICATED:                    result = "The operation being requested was not performed because the user has not been authenticated."; break;
		case ERROR_NOT_LOGGED_ON:                        result = "The operation being requested was not performed because the user has not logged on to the network. The specified service does not exist."; break;
		case ERROR_CONTINUE:                             result = "Caller to continue with work in progress."; break;
		case ERROR_ALREADY_INITIALIZED:                  result = "An attempt was made to perform an initialization operation when initialization has already been completed."; break;
		case ERROR_NO_MORE_DEVICES:                      result = "No more local devices."; break;
		case ERROR_NOT_ALL_ASSIGNED:                     result = "Not all privileges referenced are assigned to the caller."; break;
		case ERROR_SOME_NOT_MAPPED:                      result = "Some mapping between account names and security IDs was not done."; break;
		case ERROR_NO_QUOTAS_FOR_ACCOUNT:                result = "No system quota limits are specifically set for this account."; break;
		case ERROR_LOCAL_USER_SESSION_KEY:               result = "No encryption key is available. A well-known encryption key was returned."; break;
		case ERROR_NULL_LM_PASSWORD:                     result = "The password is too complex to be converted to a LAN Manager password. The LAN Manager password returned is a null string."; break;
		case ERROR_UNKNOWN_REVISION:                     result = "The revision level is unknown."; break;
		case ERROR_REVISION_MISMATCH:                    result = "Indicates two revision levels are incompatible."; break;
		case ERROR_INVALID_OWNER:                        result = "This security identifier may not be assigned as the owner of this object."; break;
		case ERROR_INVALID_PRIMARY_GROUP:                result = "This security identifier may not be assigned as the primary group of an object."; break;
		case ERROR_NO_IMPERSONATION_TOKEN:               result = "An attempt has been made to operate on an impersonation token by a thread that is not currently impersonating a client."; break;
		case ERROR_CANT_DISABLE_MANDATORY:               result = "The group cannot be disabled."; break;
		case ERROR_NO_LOGON_SERVERS:                     result = "There are currently no logon servers available to service the logon request."; break;
		case ERROR_NO_SUCH_LOGON_SESSION:                result = "A specified logon session does not exist. It may already have been terminated."; break;
		case ERROR_NO_SUCH_PRIVILEGE:                    result = "A specified privilege does not exist."; break;
		case ERROR_PRIVILEGE_NOT_HELD:                   result = "A required privilege is not held by the client."; break;
		case ERROR_INVALID_ACCOUNT_NAME:                 result = "The name provided is not a properly formed account name."; break;
		case ERROR_USER_EXISTS:                          result = "The specified user already exists."; break;
		case ERROR_NO_SUCH_USER:                         result = "The specified user does not exist."; break;
		case ERROR_GROUP_EXISTS:                         result = "The specified group already exists."; break;
		case ERROR_NO_SUCH_GROUP:                        result = "The specified group does not exist."; break;
		case ERROR_MEMBER_IN_GROUP:                      result = "Either the specified user account is already a member of the specified group, or the specified group cannot be deleted because it contains a member."; break;
		case ERROR_MEMBER_NOT_IN_GROUP:                  result = "The specified user account is not a member of the specified group account."; break;
		case ERROR_LAST_ADMIN:                           result = "The last remaining administration account cannot be disabled or deleted."; break;
		case ERROR_WRONG_PASSWORD:                       result = "Unable to update the password. The value provided as the current password is incorrect."; break;
		case ERROR_ILL_FORMED_PASSWORD:                  result = "Unable to update the password. The value provided for the new password contains values that are not allowed in passwords."; break;
		case ERROR_PASSWORD_RESTRICTION:                 result = "Unable to update the password because a password update rule has been violated."; break;
		case ERROR_LOGON_FAILURE:                        result = "Logon failure—unknown user name or bad password."; break;
		case ERROR_ACCOUNT_RESTRICTION:                  result = "Logon failure—user account restriction."; break;
		case ERROR_INVALID_LOGON_HOURS:                  result = "Logon failure—account logon time restriction violation."; break;
		case ERROR_INVALID_WORKSTATION:                  result = "Logon failure—user not allowed to log on to this computer."; break;
		case ERROR_PASSWORD_EXPIRED:                     result = "Logon failure—the specified account password has expired."; break;
		case ERROR_ACCOUNT_DISABLED:                     result = "Logon failure—account currently disabled."; break;
		case ERROR_NONE_MAPPED:                          result = "No mapping between account names and security IDs was done."; break;
		case ERROR_TOO_MANY_LUIDS_REQUESTED:             result = "Too many LUIDs were requested at one time."; break;
		case ERROR_LUIDS_EXHAUSTED:                      result = "No more LUIDs are available."; break;
		case ERROR_INVALID_SUB_AUTHORITY:                result = "The subauthority part of a security identifier is invalid for this particular use."; break;
		case ERROR_INVALID_ACL:                          result = "The access control list (ACL) structure is invalid."; break;
		case ERROR_INVALID_SID:                          result = "The security identifier structure is invalid."; break;
		case ERROR_INVALID_SECURITY_DESCR:               result = "The security descriptor structure is invalid."; break;
		case ERROR_BAD_INHERITANCE_ACL:                  result = "The inherited access control list (ACL) or access control entry (ACE) could not be built."; break;
		case ERROR_SERVER_DISABLED:                      result = "The server is currently disabled."; break;
		case ERROR_SERVER_NOT_DISABLED:                  result = "The server is currently enabled."; break;
		case ERROR_INVALID_ID_AUTHORITY:                 result = "The value provided was an invalid value for an identifier authority."; break;
		case ERROR_ALLOTTED_SPACE_EXCEEDED:              result = "No more memory is available for security information updates."; break;
		case ERROR_INVALID_GROUP_ATTRIBUTES:             result = "The specified attributes are invalid, or incompatible with the attributes for the group as a whole."; break;
		case ERROR_BAD_IMPERSONATION_LEVEL:              result = "Either a required impersonation level was not provided, or the provided impersonation level is invalid."; break;
		case ERROR_CANT_OPEN_ANONYMOUS:                  result = "Cannot open an anonymous level security token."; break;
		case ERROR_BAD_VALIDATION_CLASS:                 result = "The validation information class requested was invalid."; break;
		case ERROR_BAD_TOKEN_TYPE:                       result = "The type of the token is inappropriate for its attempted use."; break;
		case ERROR_NO_SECURITY_ON_OBJECT:                result = "Unable to perform a security operation on an object that has no associated security."; break;
		case ERROR_CANT_ACCESS_DOMAIN_INFO:              result = "Indicates that a Windows NT Server could not be contacted or that objects within the domain are protected such that necessary information could not be retrieved."; break;
		case ERROR_INVALID_SERVER_STATE:                 result = "The security account manager (SAM) or local security authority (LSA) server was in the wrong state to perform the security operation."; break;
		case ERROR_INVALID_DOMAIN_STATE:                 result = "The domain was in the wrong state to perform the security operation."; break;
		case ERROR_INVALID_DOMAIN_ROLE:                  result = "This operation is only allowed for the Primary Domain Controller (PDC) of the domain."; break;
		case ERROR_NO_SUCH_DOMAIN:                       result = "The specified domain did not exist."; break;
		case ERROR_DOMAIN_EXISTS:                        result = "The specified domain already exists."; break;
		case ERROR_DOMAIN_LIMIT_EXCEEDED:                result = "An attempt was made to exceed the limit on the number of domains per server."; break;
		case ERROR_INTERNAL_DB_CORRUPTION:               result = "Unable to complete the requested operation because of either a catastrophic media failure or a data structure corruption on the disk."; break;
		case ERROR_INTERNAL_ERROR:                       result = "The security account database contains an internal inconsistency."; break;
		case ERROR_GENERIC_NOT_MAPPED:                   result = "Generic access types were contained in an access mask that should already be mapped to nongeneric types."; break;
		case ERROR_BAD_DESCRIPTOR_FORMAT:                result = "A security descriptor is not in the right format (absolute or self-relative)."; break;
		case ERROR_NOT_LOGON_PROCESS:                    result = "The requested action is restricted for use by logon processes only. The calling process has not registered as a logon process."; break;
		case ERROR_LOGON_SESSION_EXISTS:                 result = "Cannot start a new logon session with an identifier that is already in use."; break;
		case ERROR_NO_SUCH_PACKAGE:                      result = "A specified authentication package is unknown."; break;
		case ERROR_BAD_LOGON_SESSION_STATE:              result = "The logon session is not in a state that is consistent with the requested operation."; break;
		case ERROR_LOGON_SESSION_COLLISION:              result = "The logon session identifier is already in use."; break;
		case ERROR_INVALID_LOGON_TYPE:                   result = "A logon request contained an invalid logon type value."; break;
		case ERROR_CANNOT_IMPERSONATE:                   result = "Unable to impersonate using a named pipe until data has been read from that pipe."; break;
		case ERROR_RXACT_INVALID_STATE:                  result = "The transaction state of a registry subtree is incompatible with the requested operation."; break;
		case ERROR_RXACT_COMMIT_FAILURE:                 result = "An internal security database corruption has been encountered."; break;
		case ERROR_SPECIAL_ACCOUNT:                      result = "Cannot perform this operation on built-in accounts."; break;
		case ERROR_SPECIAL_GROUP:                        result = "Cannot perform this operation on this built-in special group."; break;
		case ERROR_SPECIAL_USER:                         result = "Cannot perform this operation on this built-in special user."; break;
		case ERROR_MEMBERS_PRIMARY_GROUP:                result = "The user cannot be removed from a group because the group is currently the user's primary group."; break;
		case ERROR_TOKEN_ALREADY_IN_USE:                 result = "The token is already in use as a primary token."; break;
		case ERROR_NO_SUCH_ALIAS:                        result = "The specified local group does not exist."; break;
		case ERROR_MEMBER_NOT_IN_ALIAS:                  result = "The specified account name is not a member of the local group."; break;
		case ERROR_MEMBER_IN_ALIAS:                      result = "The specified account name is already a member of the local group."; break;
		case ERROR_ALIAS_EXISTS:                         result = "The specified local group already exists."; break;
		case ERROR_LOGON_NOT_GRANTED:                    result = "Logon failure—the user has not been granted the requested logon type at this computer."; break;
		case ERROR_TOO_MANY_SECRETS:                     result = "The maximum number of secrets that may be stored in a single system has been exceeded."; break;
		case ERROR_SECRET_TOO_LONG:                      result = "The length of a secret exceeds the maximum length allowed."; break;
		case ERROR_INTERNAL_DB_ERROR:                    result = "The local security authority database contains an internal inconsistency."; break;
		case ERROR_TOO_MANY_CONTEXT_IDS:                 result = "During a logon attempt, the user's security context accumulated too many security IDs."; break;
		case ERROR_LOGON_TYPE_NOT_GRANTED:               result = "Logon failure—the user has not been granted the requested logon type at this computer."; break;
		case ERROR_NT_CROSS_ENCRYPTION_REQUIRED:         result = "A cross-encrypted password is necessary to change a user password."; break;
		case ERROR_NO_SUCH_MEMBER:                       result = "A new member could not be added to a local group because the member does not exist."; break;
		case ERROR_INVALID_MEMBER:                       result = "A new member could not be added to a local group because the member has the wrong account type."; break;
		case ERROR_TOO_MANY_SIDS:                        result = "Too many security IDs have been specified."; break;
		case ERROR_LM_CROSS_ENCRYPTION_REQUIRED:         result = "A cross-encrypted password is necessary to change this user password."; break;
		case ERROR_NO_INHERITANCE:                       result = "Indicates an ACL contains no inheritable components."; break;
		case ERROR_FILE_CORRUPT:                         result = "The file or directory is corrupted and non-readable."; break;
		case ERROR_DISK_CORRUPT:                         result = "The disk structure is corrupted and non-readable."; break;
		case ERROR_NO_USER_SESSION_KEY:                  result = "There is no user session key for the specified logon session."; break;
		case ERROR_LICENSE_QUOTA_EXCEEDED:               result = "The service being accessed is licensed for a particular number of connections. No more connections can be made to the service at this time because there are already as many connections as the service can accept."; break;
		case ERROR_INVALID_WINDOW_HANDLE:                result = "Invalid window handle."; break;
		case ERROR_INVALID_MENU_HANDLE:                  result = "Invalid menu handle."; break;
		case ERROR_INVALID_CURSOR_HANDLE:                result = "Invalid cursor handle."; break;
		case ERROR_INVALID_ACCEL_HANDLE:                 result = "Invalid accelerator table handle."; break;
		case ERROR_INVALID_HOOK_HANDLE:                  result = "Invalid hook handle."; break;
		case ERROR_INVALID_DWP_HANDLE:                   result = "Invalid handle to a multiple-window position structure."; break;
		case ERROR_TLW_WITH_WSCHILD:                     result = "Cannot create a top-level child window."; break;
		case ERROR_CANNOT_FIND_WND_CLASS:                result = "Cannot find window class."; break;
		case ERROR_WINDOW_OF_OTHER_THREAD:               result = "Invalid window, it belongs to another thread."; break;
		case ERROR_HOTKEY_ALREADY_REGISTERED:            result = "Hot key is already registered."; break;
		case ERROR_CLASS_ALREADY_EXISTS:                 result = "Class already exists."; break;
		case ERROR_CLASS_DOES_NOT_EXIST:                 result = "Class does not exist."; break;
		case ERROR_CLASS_HAS_WINDOWS:                    result = "Class still has open windows."; break;
		case ERROR_INVALID_INDEX:                        result = "Invalid index."; break;
		case ERROR_INVALID_ICON_HANDLE:                  result = "Invalid icon handle."; break;
		case ERROR_PRIVATE_DIALOG_INDEX:                 result = "Using private DIALOG window words."; break;
		case ERROR_LISTBOX_ID_NOT_FOUND:                 result = "The list box identifier was not found."; break;
		case ERROR_NO_WILDCARD_CHARACTERS:               result = "No wildcards were found."; break;
		case ERROR_CLIPBOARD_NOT_OPEN:                   result = "Thread does not have a clipboard open."; break;
		case ERROR_HOTKEY_NOT_REGISTERED:                result = "Hot key is not registered."; break;
		case ERROR_WINDOW_NOT_DIALOG:                    result = "The window is not a valid dialog window."; break;
		case ERROR_CONTROL_ID_NOT_FOUND:                 result = "Control identifier not found."; break;
		case ERROR_INVALID_COMBOBOX_MESSAGE:             result = "Invalid message for a combo box because it does not have an edit control."; break;
		case ERROR_WINDOW_NOT_COMBOBOX:                  result = "The window is not a combo box."; break;
		case ERROR_INVALID_EDIT_HEIGHT:                  result = "Height must be less than 256."; break;
		case ERROR_DC_NOT_FOUND:                         result = "Invalid device context (DC) handle."; break;
		case ERROR_INVALID_HOOK_FILTER:                  result = "Invalid hook procedure type."; break;
		case ERROR_INVALID_FILTER_PROC:                  result = "Invalid hook procedure."; break;
		case ERROR_HOOK_NEEDS_HMOD:                      result = "Cannot set nonlocal hook without a module handle."; break;
		case ERROR_GLOBAL_ONLY_HOOK:                     result = "This hook procedure can only be set globally."; break;
		case ERROR_JOURNAL_HOOK_SET:                     result = "The journal hook procedure is already installed."; break;
		case ERROR_HOOK_NOT_INSTALLED:                   result = "The hook procedure is not installed."; break;
		case ERROR_INVALID_LB_MESSAGE:                   result = "Invalid message for single-selection list box."; break;
		case ERROR_LB_WITHOUT_TABSTOPS:                  result = "This list box does not support tab stops."; break;
		case ERROR_DESTROY_OBJECT_OF_OTHER_THREAD:       result = "Cannot destroy object created by another thread."; break;
		case ERROR_CHILD_WINDOW_MENU:                    result = "Child windows cannot have menus."; break;
		case ERROR_NO_SYSTEM_MENU:                       result = "The window does not have a system menu."; break;
		case ERROR_INVALID_MSGBOX_STYLE:                 result = "Invalid message box style."; break;
		case ERROR_INVALID_SPI_VALUE:                    result = "Invalid system-wide (SPI_*) parameter."; break;
		case ERROR_SCREEN_ALREADY_LOCKED:                result = "Screen already locked."; break;
		case ERROR_HWNDS_HAVE_DIFF_PARENT:               result = "All handles to windows in a multiple-window position structure must have the same parent."; break;
		case ERROR_NOT_CHILD_WINDOW:                     result = "The window is not a child window."; break;
		case ERROR_INVALID_GW_COMMAND:                   result = "Invalid GW_* command."; break;
		case ERROR_INVALID_THREAD_ID:                    result = "Invalid thread identifier."; break;
		case ERROR_NON_MDICHILD_WINDOW:                  result = "Cannot process a message from a window that is not a multiple-document interface (MDI) window."; break;
		case ERROR_POPUP_ALREADY_ACTIVE:                 result = "Pop-up menu already active."; break;
		case ERROR_NO_SCROLLBARS:                        result = "The window does not have scroll bars."; break;
		case ERROR_INVALID_SCROLLBAR_RANGE:              result = "Scroll bar range cannot be greater than 0x7FFF."; break;
		case ERROR_INVALID_SHOWWIN_COMMAND:              result = "Cannot show or remove the window in the way specified."; break;
		case ERROR_NO_SYSTEM_RESOURCES:                  result = "Insufficient system resources exist to complete the requested service."; break;
		case ERROR_NONPAGED_SYSTEM_RESOURCES:            result = "Insufficient system resources exist to complete the requested service."; break;
		case ERROR_PAGED_SYSTEM_RESOURCES:               result = "Insufficient system resources exist to complete the requested service."; break;
		case ERROR_WORKING_SET_QUOTA:                    result = "Insufficient quota to complete the requested service."; break;
		case ERROR_PAGEFILE_QUOTA:                       result = "Insufficient quota to complete the requested service."; break;
		case ERROR_COMMITMENT_LIMIT:                     result = "The paging file is too small for this operation to complete."; break;
		case ERROR_MENU_ITEM_NOT_FOUND:                  result = "A menu item was not found."; break;
		case ERROR_INVALID_KEYBOARD_HANDLE:              result = "Invalid keyboard layout handle."; break;
		case ERROR_HOOK_TYPE_NOT_ALLOWED:                result = "Hook type not allowed."; break;
		case ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION:   result = "This operation requires an interactive window station."; break;
		case ERROR_TIMEOUT:                              result = "This operation returned because the time-out period expired."; break;
		case ERROR_EVENTLOG_FILE_CORRUPT:                result = "The event tracking file is corrupted."; break;
		case ERROR_EVENTLOG_CANT_START:                  result = "No event tracking file could be opened, so the event tracking service did not start."; break;
		case ERROR_LOG_FILE_FULL:                        result = "The event tracking file is full."; break;
		case ERROR_EVENTLOG_FILE_CHANGED:                result = "The event tracking file has changed between read operations."; break;
		case RPC_S_INVALID_STRING_BINDING:               result = "The string binding is invalid."; break;
		case RPC_S_WRONG_KIND_OF_BINDING:                result = "The binding handle is not the correct type."; break;
		case RPC_S_INVALID_BINDING:                      result = "The binding handle is invalid."; break;
		case RPC_S_PROTSEQ_NOT_SUPPORTED:                result = "The RPC protocol sequence is not supported."; break;
		case RPC_S_INVALID_RPC_PROTSEQ:                  result = "The RPC protocol sequence is invalid."; break;
		case RPC_S_INVALID_STRING_UUID:                  result = "The string universal unique identifier (UUID) is invalid."; break;
		case RPC_S_INVALID_ENDPOINT_FORMAT:              result = "The endpoint format is invalid."; break;
		case RPC_S_INVALID_NET_ADDR:                     result = "The network address is invalid."; break;
		case RPC_S_NO_ENDPOINT_FOUND:                    result = "No endpoint was found."; break;
		case RPC_S_INVALID_TIMEOUT:                      result = "The time-out value is invalid."; break;
		case RPC_S_OBJECT_NOT_FOUND:                     result = "The object universal unique identifier (UUID) was not found."; break;
		case RPC_S_ALREADY_REGISTERED:                   result = "The object universally unique identifier (UUID) has already been registered."; break;
		case RPC_S_TYPE_ALREADY_REGISTERED:              result = "The type UUID has already been registered."; break;
		case RPC_S_ALREADY_LISTENING:                    result = "The remote procedure call (RPC) server is already listening."; break;
		case RPC_S_NO_PROTSEQS_REGISTERED:               result = "No protocol sequences have been registered."; break;
		case RPC_S_NOT_LISTENING:                        result = "The RPC server is not listening."; break;
		case RPC_S_UNKNOWN_MGR_TYPE:                     result = "The manager type is unknown."; break;
		case RPC_S_UNKNOWN_IF:                           result = "The interface is unknown."; break;
		case RPC_S_NO_BINDINGS:                          result = "There are no bindings."; break;
		case RPC_S_NO_PROTSEQS:                          result = "There are no protocol sequences."; break;
		case RPC_S_CANT_CREATE_ENDPOINT:                 result = "The endpoint cannot be created."; break;
		case RPC_S_OUT_OF_RESOURCES:                     result = "Not enough resources are available to complete this operation."; break;
		case RPC_S_SERVER_UNAVAILABLE:                   result = "The RPC server is unavailable."; break;
		case RPC_S_SERVER_TOO_BUSY:                      result = "The RPC server is too busy to complete this operation."; break;
		case RPC_S_INVALID_NETWORK_OPTIONS:              result = "The network options are invalid."; break;
		case RPC_S_NO_CALL_ACTIVE:                       result = "There is not a remote procedure call active in this thread."; break;
		case RPC_S_CALL_FAILED:                          result = "The remote procedure call failed."; break;
		case RPC_S_CALL_FAILED_DNE:                      result = "The remote procedure call failed and did not execute."; break;
		case RPC_S_PROTOCOL_ERROR:                       result = "A remote procedure call (RPC) protocol error occurred."; break;
		case RPC_S_UNSUPPORTED_TRANS_SYN:                result = "The transfer syntax is not supported by the RPC server."; break;
		case RPC_S_UNSUPPORTED_TYPE:                     result = "The universal unique identifier (UUID) type is not supported."; break;
		case RPC_S_INVALID_TAG:                          result = "The tag is invalid."; break;
		case RPC_S_INVALID_BOUND:                        result = "The array bounds are invalid."; break;
		case RPC_S_NO_ENTRY_NAME:                        result = "The binding does not contain an entry name."; break;
		case RPC_S_INVALID_NAME_SYNTAX:                  result = "The name syntax is invalid."; break;
		case RPC_S_UNSUPPORTED_NAME_SYNTAX:              result = "The name syntax is not supported."; break;
		case RPC_S_UUID_NO_ADDRESS:                      result = "No network address is available to use to construct a universal unique identifier (UUID)."; break;
		case RPC_S_DUPLICATE_ENDPOINT:                   result = "The endpoint is a duplicate."; break;
		case RPC_S_UNKNOWN_AUTHN_TYPE:                   result = "The authentication type is unknown."; break;
		case RPC_S_MAX_CALLS_TOO_SMALL:                  result = "The maximum number of calls is too small."; break;
		case RPC_S_STRING_TOO_LONG:                      result = "The string is too long."; break;
		case RPC_S_PROTSEQ_NOT_FOUND:                    result = "The RPC protocol sequence was not found."; break;
		case RPC_S_PROCNUM_OUT_OF_RANGE:                 result = "The procedure number is out of range."; break;
		case RPC_S_BINDING_HAS_NO_AUTH:                  result = "The binding does not contain any authentication information."; break;
		case RPC_S_UNKNOWN_AUTHN_SERVICE:                result = "The authentication service is unknown."; break;
		case RPC_S_UNKNOWN_AUTHN_LEVEL:                  result = "The authentication level is unknown."; break;
		case RPC_S_INVALID_AUTH_IDENTITY:                result = "The security context is invalid."; break;
		case RPC_S_UNKNOWN_AUTHZ_SERVICE:                result = "The authorization service is unknown."; break;
		case EPT_S_INVALID_ENTRY:                        result = "The entry is invalid."; break;
		case EPT_S_CANT_PERFORM_OP:                      result = "The server endpoint cannot perform the operation."; break;
		case EPT_S_NOT_REGISTERED:                       result = "There are no more endpoints available from the endpoint mapper."; break;
		case RPC_S_NOTHING_TO_EXPORT:                    result = "No interfaces have been exported."; break;
		case RPC_S_INCOMPLETE_NAME:                      result = "The entry name is incomplete."; break;
		case RPC_S_INVALID_VERS_OPTION:                  result = "The version option is invalid."; break;
		case RPC_S_NO_MORE_MEMBERS:                      result = "There are no more members."; break;
		case RPC_S_NOT_ALL_OBJS_UNEXPORTED:              result = "There is nothing to unexport."; break;
		case RPC_S_INTERFACE_NOT_FOUND:                  result = "The interface was not found."; break;
		case RPC_S_ENTRY_ALREADY_EXISTS:                 result = "The entry already exists."; break;
		case RPC_S_ENTRY_NOT_FOUND:                      result = "The entry is not found."; break;
		case RPC_S_NAME_SERVICE_UNAVAILABLE:             result = "The name service is unavailable."; break;
		case RPC_S_INVALID_NAF_ID:                       result = "The network address family is invalid."; break;
		case RPC_S_CANNOT_SUPPORT:                       result = "The requested operation is not supported."; break;
		case RPC_S_NO_CONTEXT_AVAILABLE:                 result = "No security context is available to allow impersonation."; break;
		case RPC_S_INTERNAL_ERROR:                       result = "An internal error occurred in a remote procedure call (RPC)."; break;
		case RPC_S_ZERO_DIVIDE:                          result = "The RPC server attempted an integer division by zero."; break;
		case RPC_S_ADDRESS_ERROR:                        result = "An addressing error occurred in the RPC server."; break;
		case RPC_S_FP_DIV_ZERO:                          result = "A floating-point operation at the RPC server caused a division by zero."; break;
		case RPC_S_FP_UNDERFLOW:                         result = "A floating-point underflow occurred at the RPC server."; break;
		case RPC_S_FP_OVERFLOW:                          result = "A floating-point overflow occurred at the RPC server."; break;
		case RPC_X_NO_MORE_ENTRIES:                      result = "The list of RPC servers available for the binding of auto handles has been exhausted."; break;
		case RPC_X_SS_CHAR_TRANS_OPEN_FAIL:              result = "Unable to open the character translation table file."; break;
		case RPC_X_SS_CHAR_TRANS_SHORT_FILE:             result = "The file containing the character translation table has fewer than 512 bytes."; break;
		case RPC_X_SS_IN_NULL_CONTEXT:                   result = "A null context handle was passed from the client to the host during a remote procedure call."; break;
		case RPC_X_SS_CONTEXT_DAMAGED:                   result = "The context handle changed during a remote procedure call."; break;
		case RPC_X_SS_HANDLES_MISMATCH:                  result = "The binding handles passed to a remote procedure call do not match."; break;
		case RPC_X_SS_CANNOT_GET_CALL_HANDLE:            result = "The stub is unable to get the remote procedure call handle."; break;
		case RPC_X_NULL_REF_POINTER:                     result = "A null reference pointer was passed to the stub."; break;
		case RPC_X_ENUM_VALUE_OUT_OF_RANGE:              result = "The enumeration value is out of range."; break;
		case RPC_X_BYTE_COUNT_TOO_SMALL:                 result = "The byte count is too small."; break;
		case RPC_X_BAD_STUB_DATA:                        result = "The stub received bad data."; break;
		case ERROR_INVALID_USER_BUFFER:                  result = "The supplied user buffer is not valid for the requested operation."; break;
		case ERROR_UNRECOGNIZED_MEDIA:                   result = "The disk media is not recognized. It may not be formatted."; break;
		case ERROR_NO_TRUST_LSA_SECRET:                  result = "The workstation does not have a trust secret."; break;
		case ERROR_NO_TRUST_SAM_ACCOUNT:                 result = "The SAM database on the Windows NT Server does not have a computer account for this workstation trust relationship."; break;
		case ERROR_TRUSTED_DOMAIN_FAILURE:               result = "The trust relationship between the primary domain and the trusted domain failed."; break;
		case ERROR_TRUSTED_RELATIONSHIP_FAILURE:         result = "The trust relationship between this workstation and the primary domain failed."; break;
		case ERROR_TRUST_FAILURE:                        result = "The network logon failed."; break;
		case RPC_S_CALL_IN_PROGRESS:                     result = "A remote procedure call is already in progress for this thread."; break;
		case ERROR_NETLOGON_NOT_STARTED:                 result = "An attempt was made to logon, but the network logon service was not started."; break;
		case ERROR_ACCOUNT_EXPIRED:                      result = "The user's account has expired."; break;
		case ERROR_REDIRECTOR_HAS_OPEN_HANDLES:          result = "The redirector is in use and cannot be unloaded."; break;
		case ERROR_PRINTER_DRIVER_ALREADY_INSTALLED:     result = "The specified printer driver is already installed."; break;
		case ERROR_UNKNOWN_PORT:                         result = "The specified port is unknown."; break;
		case ERROR_UNKNOWN_PRINTER_DRIVER:               result = "The printer driver is unknown."; break;
		case ERROR_UNKNOWN_PRINTPROCESSOR:               result = "The print processor is unknown."; break;
		case ERROR_INVALID_SEPARATOR_FILE:               result = "The specified separator file is invalid."; break;
		case ERROR_INVALID_PRIORITY:                     result = "The specified priority is invalid."; break;
		case ERROR_INVALID_PRINTER_NAME:                 result = "The printer name is invalid."; break;
		case ERROR_PRINTER_ALREADY_EXISTS:               result = "The printer already exists."; break;
		case ERROR_INVALID_PRINTER_COMMAND:              result = "The printer command is invalid."; break;
		case ERROR_INVALID_DATATYPE:                     result = "The specified data type is invalid."; break;
		case ERROR_INVALID_ENVIRONMENT:                  result = "The environment specified is invalid."; break;
		case RPC_S_NO_MORE_BINDINGS:                     result = "There are no more bindings."; break;
		case ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT:    result = "The account used is an interdomain trust account. Use your global user account or local user account to access this server."; break;
		case ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT:    result = "The account used is a computer account. Use your global user account or local user account to access this server."; break;
		case ERROR_NOLOGON_SERVER_TRUST_ACCOUNT:         result = "The account used is a server trust account. Use your global user account or local user account to access this server."; break;
		case ERROR_DOMAIN_TRUST_INCONSISTENT:            result = "The name or security identifier (SID) of the domain specified is inconsistent with the trust information for that domain."; break;
		case ERROR_SERVER_HAS_OPEN_HANDLES:              result = "The server is in use and cannot be unloaded."; break;
		case ERROR_RESOURCE_DATA_NOT_FOUND:              result = "The specified image file did not contain a resource section."; break;
		case ERROR_RESOURCE_TYPE_NOT_FOUND:              result = "The specified resource type cannot be found in the image file."; break;
		case ERROR_RESOURCE_NAME_NOT_FOUND:              result = "The specified resource name cannot be found in the image file."; break;
		case ERROR_RESOURCE_LANG_NOT_FOUND:              result = "The specified resource language identifier cannot be found in the image file."; break;
		case ERROR_NOT_ENOUGH_QUOTA:                     result = "Not enough quota is available to process this command."; break;
		case RPC_S_NO_INTERFACES:                        result = "No interfaces have been registered."; break;
		case RPC_S_CALL_CANCELLED:                       result = "The server was altered while processing this call."; break;
		case RPC_S_BINDING_INCOMPLETE:                   result = "The binding handle does not contain all required information."; break;
		case RPC_S_COMM_FAILURE:                         result = "Communications failure."; break;
		case RPC_S_UNSUPPORTED_AUTHN_LEVEL:              result = "The requested authentication level is not supported."; break;
		case RPC_S_NO_PRINC_NAME:                        result = "No principal name registered."; break;
		case RPC_S_NOT_RPC_ERROR:                        result = "The error specified is not a valid Windows NT RPC error value."; break;
		case RPC_S_UUID_LOCAL_ONLY:                      result = "A UUID that is valid only on this computer has been allocated."; break;
		case RPC_S_SEC_PKG_ERROR:                        result = "A security package specific error occurred."; break;
		case RPC_S_NOT_CANCELLED:                        result = "Thread is not canceled."; break;
		case RPC_X_INVALID_ES_ACTION:                    result = "Invalid operation on the encoding/decoding handle."; break;
		case RPC_X_WRONG_ES_VERSION:                     result = "Incompatible version of the serializing package."; break;
		case RPC_X_WRONG_STUB_VERSION:                   result = "Incompatible version of the RPC stub."; break;
		case RPC_X_INVALID_PIPE_OBJECT:                  result = "The idl pipe object is invalid or corrupted."; break;
		case RPC_X_INVALID_PIPE_OPERATION:               result = "The operation is invalid for a given idl pipe object."; break;
		case RPC_X_WRONG_PIPE_VERSION:                   result = "The Interface Definition Language (IDL) pipe version is not supported."; break;
		case RPC_S_GROUP_MEMBER_NOT_FOUND:               result = "The group member was not found."; break;
		case EPT_S_CANT_CREATE:                          result = "The endpoint mapper database could not be created."; break;
		case RPC_S_INVALID_OBJECT:                       result = "The object UUID is the nil UUID."; break;
		case ERROR_INVALID_TIME:                         result = "The specified time is invalid."; break;
		case ERROR_INVALID_FORM_NAME:                    result = "The specified form name is invalid."; break;
		case ERROR_INVALID_FORM_SIZE:                    result = "The specified form size is invalid."; break;
		case ERROR_ALREADY_WAITING:                      result = "The specified printer handle is already being waited on"; break;
		case ERROR_PRINTER_DELETED:                      result = "The specified printer has been deleted."; break;
		case ERROR_INVALID_PRINTER_STATE:                result = "The state of the printer is invalid."; break;
		case ERROR_PASSWORD_MUST_CHANGE:                 result = "The user must change his password before he logs on the first time."; break;
		case ERROR_DOMAIN_CONTROLLER_NOT_FOUND:          result = "Could not find the domain controller for this domain."; break;
		case ERROR_ACCOUNT_LOCKED_OUT:                   result = "The referenced account is currently locked out and may not be logged on to."; break;
		case OR_INVALID_OXID:                            result = "The object exporter specified was not found."; break;
		case OR_INVALID_OID:                             result = "The object specified was not found."; break;
		case OR_INVALID_SET:                             result = "The object resolver set specified was not found."; break;
		case RPC_S_SEND_INCOMPLETE:                      result = "Some data remains to be sent in the request buffer."; break;
		case ERROR_INVALID_PIXEL_FORMAT:                 result = "The pixel format is invalid."; break;
		case ERROR_BAD_DRIVER:                           result = "The specified driver is invalid."; break;
		case ERROR_INVALID_WINDOW_STYLE:                 result = "The window style or class attribute is invalid for this operation."; break;
		case ERROR_METAFILE_NOT_SUPPORTED:               result = "The requested metafile operation is not supported."; break;
		case ERROR_TRANSFORM_NOT_SUPPORTED:              result = "The requested transformation operation is not supported."; break;
		case ERROR_CLIPPING_NOT_SUPPORTED:               result = "The requested clipping operation is not supported."; break;
		case ERROR_BAD_USERNAME:                         result = "The specified user name is invalid."; break;
		case ERROR_NOT_CONNECTED:                        result = "This network connection does not exist."; break;
		case ERROR_OPEN_FILES:                           result = "This network connection has files open or requests pending."; break;
		case ERROR_ACTIVE_CONNECTIONS:                   result = "Active connections still exist."; break;
		case ERROR_DEVICE_IN_USE:                        result = "The device is in use by an active process and cannot be disconnected."; break;
		case ERROR_UNKNOWN_PRINT_MONITOR:                result = "The specified print monitor is unknown."; break;
		case ERROR_PRINTER_DRIVER_IN_USE:                result = "The specified printer driver is currently in use."; break;
		case ERROR_SPOOL_FILE_NOT_FOUND:                 result = "The spool file was not found."; break;
		case ERROR_SPL_NO_STARTDOC:                      result = "A StartDocPrinter call was not issued."; break;
		case ERROR_SPL_NO_ADDJOB:                        result = "An AddJob call was not issued."; break;
		case ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED:    result = "The specified print processor has already been installed."; break;
		case ERROR_PRINT_MONITOR_ALREADY_INSTALLED:      result = "The specified print monitor has already been installed."; break;
		case ERROR_INVALID_PRINT_MONITOR:                result = "The specified print monitor does not have the required functions."; break;
		case ERROR_PRINT_MONITOR_IN_USE:                 result = "The specified print monitor is currently in use."; break;
		case ERROR_PRINTER_HAS_JOBS_QUEUED:              result = "The requested operation is not allowed when there are jobs queued to the printer."; break;
		case ERROR_SUCCESS_REBOOT_REQUIRED:              result = "The requested operation is successful. Changes will not be effective until the system is rebooted."; break;
		case ERROR_SUCCESS_RESTART_REQUIRED:             result = "The requested operation is successful. Changes will not be effective until the service is restarted."; break;
		case ERROR_WINS_INTERNAL:                        result = "WINS encountered an error while processing the command."; break;
		case ERROR_CAN_NOT_DEL_LOCAL_WINS:               result = "The local WINS can not be deleted."; break;
		case ERROR_STATIC_INIT:                          result = "The importation from the file failed."; break;
		case ERROR_INC_BACKUP:                           result = "The backup failed. Was a full backup done before?"; break;
		case ERROR_FULL_BACKUP:                          result = "The backup failed. Check the directory to which you are backing the database."; break;
		case ERROR_REC_NON_EXISTENT:                     result = "The name does not exist in the WINS database."; break;
		case ERROR_RPL_NOT_ALLOWED:                      result = "Replication with a nonconfigured partner is not allowed."; break;
		case ERROR_NO_BROWSER_SERVERS_FOUND:             result = "The list of servers for this workgroup is not currently available."; break;
		
		default: 
			synce_trace("Unknown error code: 0x%08x", error);
			result = "Unknown error";
			break;
	}

	return result;
}

/** @} */
