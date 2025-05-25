savedcmd_/home/neg/syscall_logger/syscall_logger.mod := printf '%s\n'   syscall_logger.o | awk '!x[$$0]++ { print("/home/neg/syscall_logger/"$$0) }' > /home/neg/syscall_logger/syscall_logger.mod
