cmd_/home/deadpool/Desktop/Programming/MyWork/user-land-filesystem/driver/Module.symvers := sed 's/ko$$/o/' /home/deadpool/Desktop/Programming/MyWork/user-land-filesystem/driver/modules.order | scripts/mod/modpost -m -a   -o /home/deadpool/Desktop/Programming/MyWork/user-land-filesystem/driver/Module.symvers -e -i Module.symvers   -T -