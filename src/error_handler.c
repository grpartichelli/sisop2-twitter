void signal_error(int i , char *s){
	if(i){
        printf("%s",s);
        exit(1);
    }
}