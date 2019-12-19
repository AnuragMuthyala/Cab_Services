#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<semaphore.h>
#include<pthread.h>

#define waitState 0
#define onRidePoolOne 1
#define onRidePoolFull 2
#define onRidePremier 3
#define POOL 0
#define PREMIER 1

void init_fn();
int number(time_t,time_t);
void acceptRide(int);
void onRide(int);
void endRide(int,int);
int bookCab(int,time_t,time_t);
void assign(int,int,time_t);
void makePayment(int);
void acceptPayment(int,int);
void reader();

pthread_t tid1[100],tid2[100],tid3[100];
int dr[100],ri[100],se[100];
int m = 6,n = 2,k = 2;
int count = 0;
sem_t mutex;

struct cab{
int state;
int passengers[2];
time_t start[2];
time_t rideTime[2];
pthread_mutex_t lock;
}*c[100];

struct rider{
int alloc;
int request;
int type;
int paid;
time_t maxTime;
time_t rideTime;
}*r[100];

void reader(){
printf("Number of riders:");
scanf("%d",&m);
printf("Number of drivers:");
scanf("%d",&n);
printf("Number of servers:");
scanf("%d",&k);
}

void init_fn(){
for(int i = 0;i < m;i++){
r[i] = malloc(sizeof(struct rider));
r[i]->alloc = -1;
r[i]->type = -1;
r[i]->request = -1;
r[i]->paid = -1;
r[i]->rideTime = 0;
}
for(int i = 0;i < n;i++){
c[i] = malloc(sizeof(struct cab));
c[i]->state = waitState;
c[i]->passengers[0] = -1,c[i]->passengers[1] = -1;
c[i]->start[0] = 0,c[i]->start[1] = 0;
c[i]->rideTime[0] = 0,c[i]->rideTime[1] = 0;
pthread_mutex_init(&c[i]->lock,NULL);
}
sem_init(&mutex,0,k);
count = m;
}

void init_threads(){
for(int i = 0;i < m;i++)
pthread_join(tid1[i],NULL);
}

int number(time_t l,time_t h){
return l + (rand() % (h - l + 1));
}

void* driver_fn(void *arg){
int i = (int *)arg;
printf("driver %d has started\n",i),sleep(1);
int cl= 0;
while(1){
cl = 0;
while(c[i]->passengers[0] == -1 && c[i]->passengers[1] == -1);
if(c[i]->passengers[0] == -1 && c[i]->passengers[1] != -1){
c[i]->passengers[0] = c[i]->passengers[1];
c[i]->start[0] = c[i]->start[1];
c[i]->rideTime[0] = c[i]->rideTime[1];
c[i]->passengers[1] = -1;
c[i]->start[1] = 0;
c[i]->rideTime[1] = 0;
} 
acceptRide(i);
}
}

void acceptRide(int i){
int cl = 0;
if(r[c[i]->passengers[0]]->type == PREMIER){
c[i]->state = onRidePremier;
c[i]->start[0] = time(NULL);
c[i]->rideTime[0] = r[c[i]->passengers[0]]->rideTime;
while(time(NULL) - c[i]->start[0] < c[i]->rideTime[0]);
endRide(i,0);
}

else if(r[c[i]->passengers[0]]->type == POOL){
while(c[i]->passengers[0] != -1){
c[i]->start[0] = time(NULL);
c[i]->rideTime[0] = r[c[i]->passengers[0]]->rideTime;
c[i]->state = onRidePoolOne;
while((time(NULL) - c[i]->start[0] < c[i]->rideTime[0]) && (c[i]->passengers[1] == -1 || time(NULL) - c[i]->start[1] < c[i]->rideTime[1])){
if(c[i]->passengers[1] != -1 && c[i]->state == onRidePoolOne)
onRide(i);
}
if(time(NULL) - c[i]->start[0] >= c[i]->rideTime[0])
endRide(i,0);
if(c[i]->passengers[1] != -1 && (time(NULL) - c[i]->start[1] >= c[i]->rideTime[1]))
endRide(i,1);
}
}
}

void onRide(int i){
pthread_mutex_lock(&c[i]->lock);
c[i]->state = onRidePoolFull;
pthread_mutex_unlock(&c[i]->lock);
}

void endRide(int i,int j){
int k;
k = c[i]->passengers[j];
pthread_mutex_lock(&c[i]->lock);
if(j == 0){
if(c[i]->passengers[1] != -1){
c[i]->passengers[0] = c[i]->passengers[1];
c[i]->start[0] = c[i]->start[1];
c[i]->rideTime[0] = c[i]->rideTime[1];
c[i]->passengers[1] = -1;
c[i]->start[1] = 0;
c[i]->rideTime[1] = 0;
c[i]->state = onRidePoolOne;
}
else{
c[i]->passengers[0] = -1;
c[i]->start[0] = 0;
c[i]->rideTime[0] = 0;
c[i]->state = waitState;
}
}
else if(j == 1){
c[i]->passengers[1] = -1;
c[i]->start[1] = 0;
c[i]->rideTime[1] = 0;
c[i]->state = onRidePoolOne;
}

pthread_mutex_unlock(&c[i]->lock);
printf("Rider %d completed..\n",k),sleep(1);
r[k]->alloc = -1;
}

void* rider_fn(void *arg){
int i = (int *)arg;
printf("Rider %d has arrived\n",i),sleep(1);
int j;
r[i]->maxTime = number(0,5);
r[i]->type = number(0,1);
r[i]->rideTime = number(1,3);
j = bookCab(i,r[i]->maxTime,r[i]->rideTime);
if(j == -1){
printf("Session time out..%d\n",i),sleep(1);
}
else{
r[i]->alloc = j;
while(r[i]->alloc != -1);
makePayment(i);
}
printf("Rider %d exited..\n",i),sleep(1);
}

int bookCab(int id,time_t max,time_t ridetime){
int i;
printf("bookcab..%d\n",id),sleep(1);
time_t begin,end;
time(&begin);
do{
if(r[id]->type == PREMIER){
for(i = 0;i < n;i++){
if(c[i]->passengers[0] == -1){
pthread_mutex_lock(&c[i]->lock);
c[i]->passengers[0] = id;
pthread_mutex_unlock(&c[i]->lock);
return i;
}
}
}
else if(r[id]->type == POOL){
for(i = 0;i < n;i++){
if(c[i]->passengers[0] == -1){
pthread_mutex_lock(&c[i]->lock);
c[i]->passengers[0] = id;
pthread_mutex_unlock(&c[i]->lock);
return i;
}
else if(c[i]->passengers[1] == -1 && c[i]->state == onRidePoolOne){
pthread_mutex_lock(&c[i]->lock);
c[i]->passengers[1] = id;
pthread_mutex_unlock(&c[i]->lock);
return i; 
}
}
}
time(&end);
}while((end - begin) <= max);
return -1;
}

void makePayment(int i){
printf("makepayment..%d\n",i),sleep(1);
sem_wait(&mutex);
r[i]->paid = 0;
while(r[i]->paid == 0);
printf("Payment done successfully by %d..\n",i),sleep(1);
count--;
sem_post(&mutex);
}

void* server_fn(void *arg){
int i = (int *)arg,j = 0;
printf("Server %d has started\n",i),sleep(1);
while(1){
while(r[j]->paid != 0)
j = (j + 1)%m;
acceptPayment(i,j);
}
}

void acceptPayment(int i,int j){
printf("Server has initiated %d's transaction\n",j),sleep(1);
r[j]->paid = 2;
sleep(2);
r[j]->paid = 1;
}

void main(){

reader();
init_fn();
srand(time(NULL));

for(int i = 0;i < n;i++)
dr[i] = pthread_create(&tid2[i],NULL,driver_fn,(void *)i);
for(int i = 0;i < k;i++)
se[i] = pthread_create(&tid3[i],NULL,server_fn,(void *)i);
for(int i = 0;i < m;i++){
ri[i] = pthread_create(&tid1[i],NULL,rider_fn,(void *)i);
pthread_join(tid1[i],NULL);
sleep(1);
}

exit(0);
}
