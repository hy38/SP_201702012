#include <stdio.h>

struct student{
	char *myname;
	int mynum;
};


void swap(struct student *arg1, struct student *arg2){
	
	char *name_t;
	int num_t;
	name_t = arg1->myname;
	arg1->myname = arg2->myname;
	arg2->myname = name_t;

	num_t = arg1->mynum;
	arg1->mynum = arg2->mynum;
	arg2->mynum = num_t;

	printf("swap Function call!!\n");

}


int main(void){

	struct student shPark;
	struct student sys_TA;

	shPark.myname = "PSH";
	shPark.mynum = 201702012;

	sys_TA.myname = "JBK";
	sys_TA.mynum = 2;

	printf("myname : %s\n", shPark.myname);
	printf("mynumber : %d\n", shPark.mynum);
	printf("taname : %s\n", sys_TA.myname);
	printf("class : %d\n", sys_TA.mynum);

	printf("[before] myname : %s, taname : %s\n", shPark.myname, sys_TA.myname);
	printf("[before] mynum : %d, tanum : %d\n", shPark.mynum, sys_TA.mynum);

	swap(&shPark, &sys_TA);

	printf("[after] myname : %s, taname : %s\n", shPark.myname, sys_TA.myname);
	printf("[after] mynum : %d, tanum : %d\n", shPark.mynum, sys_TA.mynum);


}
