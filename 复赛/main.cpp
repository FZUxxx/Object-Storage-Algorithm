#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <stack>
#include <queue>
#include <vector>
#include <climits>
#include <unordered_map>
using namespace std;

#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM (3)
#define FRE_PER_SLICING (1800)
#define EXTRA_TIME (105)

typedef struct Request_ {
    int object_id;
    int prev_id;
    bool is_done;
    bool is_busy;
    int born_time;
} Request;

typedef struct Object_ {
    int replica[REP_NUM + 1];
    int* unit[REP_NUM + 1];//unit[i]是一个二维数组，unit[i][j]是第i个副本的第j块的位置
    int size;
    int category;//对象种类
    int last_request_point;
    bool is_delete;
    bool is_continuous[REP_NUM + 1];//对象副本是否连续存储在硬盘上
} Object;

Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];

int T, M, N, V, G, K;
int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
int disk_point[MAX_DISK_NUM];


void timestamp_action()
{
    int timestamp;
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);

    fflush(stdout);
}

void do_object_delete(const int* object_unit, int* disk_unit, int size)
{
    for (int i = 1; i <= size; i++) {
        disk_unit[object_unit[i]] = 0;
    }
}

void delete_action()
{
    int n_delete;
    int abort_num = 0;
    static int _id[MAX_OBJECT_NUM];

    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);
    }

    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false && request[current_id].is_busy == false) {
                abort_num++;
            }
            current_id = request[current_id].prev_id;
        }
    }

    printf("%d\n", abort_num);
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {
            if (request[current_id].is_done == false && request[current_id].is_busy == false) {
                printf("%d\n", current_id);
            }
            current_id = request[current_id].prev_id;
        }
        for (int j = 1; j <= REP_NUM; j++) {
            do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size);
        }
        
        object[id].is_delete = true;
    }

    fflush(stdout);
}

void do_object_write(int* object_unit, int* disk_unit, int size, int object_id, int replica_index)
{
    int current_write_point = 0;
    int start = -1;
    int count = 0;
    bool find = false;
    if (replica_index == 1) {//如果是1号副本，从前往后写
        for (int i = 1; i <= V; i++) {
            if (disk_unit[i] == 0) {  // 空闲块
                if (count == 0) start = i;  // 记录连续开始位置
                count++;
                if (count == size) {  // 找到足够连续空间
                    find = true;
                    break;
                }
            }
            else {  // 遇到非空块，重置计数
                count = 0;
                start = -1;
            }
        }
        if (find) {
            for (int i = 0; i < size; i++) {
                disk_unit[start+i] = object_id;
                object_unit[i+1] = start+i;
            }
        }
        else {
            for (int i = 1; i <= V; i++) {
                if (disk_unit[i] == 0) {
                    disk_unit[i] = object_id;
                    object_unit[++current_write_point] = i;
                    if (current_write_point == size) {
                        break;
                    }
                }
            }
        }

    }
    else {//如果是2号或3号副本，从后往前写
        for (int i = V; i >= 1; i--) {
            if (disk_unit[i] == 0) {
                disk_unit[i] = object_id;
                object_unit[++current_write_point] = i;
                if (current_write_point == size) {
                    break;
                }
            }
        }

    }
   
    bool is_continuous = true;
    for (int i = 2; i <= size; i++) {
        if (object_unit[i]!= object_unit[i-1]+1) {
            is_continuous = false;//如果对象的后序部分位置与前序部分位置不连续，则将其标记为不连续
        }
    }
    object[object_id].is_continuous[replica_index] = is_continuous;
    
}

void write_action()
{
    int n_write;
    scanf("%d", &n_write);
    for (int i = 1; i <= n_write; i++) {
        int id, size, category;
        scanf("%d%d%d", &id, &size, &category);
        object[id].last_request_point = 0;
        for (int j = 1; j <= REP_NUM; j++) {
            object[id].replica[j] = (id + j)%N+1;
            object[id].unit[j] = static_cast<int*>(malloc(sizeof(int) * (size + 1)));//第j个副本写在那个磁盘上的哪个地方
            object[id].size = size;
            object[id].is_delete = false;
            object[id].category = category;
            do_object_write(object[id].unit[j], disk[object[id].replica[j]], size, id, j);
        }

        printf("%d\n", id);
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", object[id].replica[j]);
            for (int k = 1; k <= size; k++) {
                printf(" %d", object[id].unit[j][k]);
            }
            printf("\n");
        }
    }

    fflush(stdout);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
vector<unordered_map<int,int>> disk_request_map(MAX_DISK_NUM); //任务map

vector<int> current_request1   (MAX_DISK_NUM); //记录每个硬盘当前处理的任务号
vector<int> current_object_id1 (MAX_DISK_NUM); //记录每个硬盘当前处理的对象
vector<int> current_replica1  (MAX_DISK_NUM); //记录每个硬盘当前处理是对象的几号副本
vector<int> current_first_pos1 (MAX_DISK_NUM); //记录每个硬盘当前处理对象的首地址

vector<int> current_haveread1  (MAX_DISK_NUM); //记录每个硬盘已经连续做了多少次读操作
vector<int> current_disk_pos1(MAX_DISK_NUM, 1); //记录每个硬盘当前磁头位置，初始化时为1
vector<int> pAss1(MAX_DISK_NUM);               //记录每个硬盘当前需要pass的次数
vector<int> jUmp1(MAX_DISK_NUM);               //记录每个硬盘当前需要jump的次数
vector<int> rEad1(MAX_DISK_NUM);               //记录每个硬盘当前需要read的次数

vector<int> current_request2(MAX_DISK_NUM); //记录每个硬盘当前处理的任务号
vector<int> current_object_id2(MAX_DISK_NUM); //记录每个硬盘当前处理的对象
vector<int> current_replica2(MAX_DISK_NUM); //记录每个硬盘当前处理是对象的几号副本
vector<int> current_first_pos2(MAX_DISK_NUM); //记录每个硬盘当前处理对象的首地址

vector<int> current_haveread2(MAX_DISK_NUM); //记录每个硬盘已经连续做了多少次读操作
vector<int> current_disk_pos2(MAX_DISK_NUM, 1); //记录每个硬盘当前磁头位置，初始化时为1
vector<int> pAss2(MAX_DISK_NUM);               //记录每个硬盘当前需要pass的次数
vector<int> jUmp2(MAX_DISK_NUM);               //记录每个硬盘当前需要jump的次数
vector<int> rEad2(MAX_DISK_NUM);               //记录每个硬盘当前需要read的次数

//把一个请求从所有已经进入的map中删除
void all_disk_map_erase(int erase_request_id) {
    int erase_object_id = request[erase_request_id].object_id;    // 找到要弹出请求的对象号
    for (int i = 1; i <= 3; i++) {
        if (object[erase_object_id].is_continuous[i]) {                 //如果i号副本是连续存储的
            int erase_disk_id = object[erase_object_id].replica[i];     // 找到i号副本的存储硬盘号
            int first_pos = object[erase_object_id].unit[i][1];         //找到i号副本首地址
            disk_request_map[erase_disk_id].erase(first_pos);           //删除
        }
    }
}


//从第i号硬盘中获取一个请求任务,当前逻辑为找离磁头最近的
void get_mission1(int i, int t) {

    int return_request = 0;
    for (int j = 0; j < V; j++) {
        int k=(j + current_disk_pos1[i] - 1) % V + 1;
        if (k >= V / 6)continue;
        if (disk_request_map[i].find(k)!= disk_request_map[i].end()&& disk_request_map[i][k]!=0) {
            if (object[request[disk_request_map[i][k]].object_id].is_delete || request[disk_request_map[i][k]].is_done || request[disk_request_map[i][k]].is_busy) {
                disk_request_map[i][k] = 0;
                continue;
            }
            return_request = disk_request_map[i][k];
            break;
        }
    }

    if (return_request == 0) {
        current_request1[i] = 0; // 重置任务状态
        current_object_id1[i] = 0;
        current_replica1[i] = 0;
        current_first_pos1[i] = 0;
        return;
    }

    
    current_request1[i]   = return_request;                       //更新该磁盘当前处理的请求号  
    current_object_id1[i] = request[current_request1[i]].object_id;//更新该磁盘当前处理的任务号
    for (int j = 1; j <= 3; j++) {
        if (object[current_object_id1[i]].replica[j] == i) {
            current_replica1[i] = j;                              //更新该磁盘当前处理的任务副本号
            break;
        }
    }
    current_first_pos1[i] = object[current_object_id1[i]].unit[current_replica1[i]][1];//更新该磁盘当前处理的任务首地址
    
    return;
}

void get_mission2(int i, int t) {

    int return_request = 0;
    for (int j = 0; j < V; j++) {
        int k = (j + current_disk_pos2[i] - 1) % V + 1;
        if (k < V / 6)continue;
        if (disk_request_map[i].find(k) != disk_request_map[i].end() && disk_request_map[i][k] != 0) {
            if (object[request[disk_request_map[i][k]].object_id].is_delete || request[disk_request_map[i][k]].is_done || request[disk_request_map[i][k]].is_busy) {
                disk_request_map[i][k] = 0;
                continue;
            }
            return_request = disk_request_map[i][k];
            break;
        }
    }

    if (return_request == 0) {
        current_request2[i] = 0; // 重置任务状态
        current_object_id2[i] = 0;
        current_replica2[i] = 0;
        current_first_pos2[i] = 0;
        return;
    }


    current_request2[i] = return_request;                       //更新该磁盘当前处理的请求号  
    current_object_id2[i] = request[current_request2[i]].object_id;//更新该磁盘当前处理的任务号
    for (int j = 1; j <= 3; j++) {
        if (object[current_object_id2[i]].replica[j] == i) {
            current_replica2[i] = j;                              //更新该磁盘当前处理的任务副本号
            break;
        }
    }
    current_first_pos2[i] = object[current_object_id2[i]].unit[current_replica2[i]][1];//更新该磁盘当前处理的任务首地址

    return;
}

//把已经连读的次数转换为下一次读的令牌消耗
int read_resume(int i) {
    if (i == 0) return 64;
    else if (i == 1) return 52;
    else if (i == 2) return 42;
    else if (i == 3) return 34;
    else if (i == 4) return 28;
    else if (i == 5) return 23;
    else if (i == 6) return 19;
    else if (i == 7) return 16;
    return 16;
}

int get_distance1(int i) {
    //int pos = (current_disk_pos1[i] - 1) % V + 1; // 确保位置在[1, V]
    int pos = current_disk_pos1[i];
    int target = current_first_pos1[i];
    if (pos <= target) return target - pos;
    else return (V - pos) + target;
}

int get_distance2(int i) {
    //int pos = (current_disk_pos2[i] - 1) % V + 1; // 确保位置在[1, V]
    int pos = current_disk_pos2[i];
    int target = current_first_pos2[i];
    if (pos <= target) return target - pos;
    else return (V - pos) + target;
}

bool not_busy(int first_pos,int i) {
    int legth = V/6;
    bool d1 = true;
    bool d2 = true;
    //int pos1 = (current_disk_pos1[i] - 1) % V + 1; // 确保位置在[1, V]
    //int pos2 = (current_disk_pos2[i] - 1) % V + 1; // 确保位置在[1, V]
    int pos1 = current_disk_pos1[i];
    int pos2 = current_disk_pos2[i];
    if (first_pos >= pos1 && first_pos - pos1 >= legth) {
        d1 = false;
    }
    if (first_pos < pos1 && V / 3 - pos1 + first_pos >= legth) {
        d1 = false;
    }
    if (first_pos >= pos2 && first_pos - pos2 >= legth) {
        d2 = false;
    }
    if (first_pos < pos2 && V / 3 - pos2 + first_pos >= legth) {
        d2 = false;
    }
    return (d1||d2);
}

unordered_map<int, vector<int>>time_to_request;

void read_action(int t)
{
    int n_read;
    int request_id, object_id;
    //每个时间步都会重置的项目
    vector<int> current_token1(MAX_DISK_NUM,0);//记录每个硬盘当前时间步消耗的令牌数
    vector<int> current_token2(MAX_DISK_NUM, 0);//记录每个硬盘当前时间步消耗的令牌数
    int nums = 0;               //记录当前时间步完成的请求个数
    queue<int> request_ans;   //记录当前时间步完成的请求

    queue<int> request_busy_ans;   //记录当前时间步busy的请求

    scanf("%d", &n_read);

    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
        request[request_id].is_busy = false;
        request[request_id].born_time = t;


        time_to_request[t].push_back(request_id);

        //如果该任务是连续储存的则让他进入map中
        if (object[object_id].is_continuous[1]&& not_busy(object[object_id].unit[1][1], object[object_id].replica[1])) { //如果该请求的任务的1号副本是连续储存的则让他进入map中
            int disk_index = object[object_id].replica[1];        //找到存储当前任务1号副本的硬盘号
            int first_pos = object[object_id].unit[1][1];         //找到当前副本首地址
            disk_request_map[disk_index][first_pos] = request_id; //把当前任务号塞进对应硬盘的map内
        }
        else {
            request_busy_ans.push(request_id);
            request[request_id].is_busy = true;
        }
    }
    
    for (int i = 1; i <= N; i++) {
        
        while (1) {
            if (current_request1[i] == 0) {             //若当前硬盘未处理任务
                get_mission1(i,t);
                if (current_request1[i] == 0) {
                    //当前硬盘没处理任务，map中也无任务，则结束
                    printf("#\n");
                    break;
                }
                all_disk_map_erase(current_request1[i]);
                if (get_distance1(i) <= G) {
                    //若当前磁头位置距离对象存储的首地址<=G,则以p+r的方式读
                    int dis= get_distance1(i); 
                    if (dis <= 8) {    
                        pAss1[i] = 0;
                        rEad1[i] = object[current_object_id1[i]].size+dis;
                    }
                    else {
                        pAss1[i] = dis;
                        rEad1[i] = object[current_object_id1[i]].size;
                    }
                }
                else {
                    //以j+r的方式读
                    jUmp1[i] = 1;
                    rEad1[i] = object[current_object_id1[i]].size;
                }
            }
            else {//当前磁盘在处理任务
                //先处理j行为
                if (jUmp1[i] > 0) {
                    if (current_token1[i]==0) {
                        current_token1[i] = G;
                        current_disk_pos1[i] = current_first_pos1[i];
                        current_haveread1[i] = 0;
                        printf("j %d\n", current_first_pos1[i]);
                        jUmp1[i]--;
                        break;
                    }
                    else {
                        printf("#\n");
                        break;
                    }
                }
                //再处理p行为
                else if (pAss1[i] > 0) {
                    if (current_token1[i] < G) {
                        current_token1[i] += 1;
                        current_disk_pos1[i] += 1;
                        current_disk_pos1[i] = (current_disk_pos1[i] - 1) % V + 1;
                        current_haveread1[i] = 0;
                        printf("p");
                        pAss1[i]--;
                    }
                    else {
                        printf("#\n");
                        break;
                    }
                }
                //再处理r行为
                else {
                    if (rEad1[i] > 0) {
                        if (G - current_token1[i] >= read_resume(current_haveread1[i])) {
                            current_token1[i] += read_resume(current_haveread1[i]);
                            current_disk_pos1[i] += 1;
                            current_disk_pos1[i] = (current_disk_pos1[i] - 1) % V + 1;
                            current_haveread1[i] += 1;                      
                            printf("r");
                            rEad1[i]--;
                        }
                        else {
                            printf("#\n");
                            break;
                        }
                    }
                    else { // 任务完成
                        if (!object[current_object_id1[i]].is_delete) {
                            int cur = current_request1[i];
                            while (cur != 0) {
                                if (!request[cur].is_done && !request[cur].is_busy) {
                                    request[cur].is_done = true;
                                    nums++;
                                    request_ans.push(cur);
                                    cur = request[cur].prev_id;
                                }
                                else {
                                    break;
                                }
                            }
                        }
                        current_request1[i] = 0; // 重置任务状态
                        current_object_id1[i] = 0;
                        current_replica1[i] = 0;
                        current_first_pos1[i] = 0;
                    }
                }
            }
        }

        while (1) {
            if (current_request2[i] == 0) {             //若当前硬盘未处理任务
                get_mission2(i,t);
                if (current_request2[i] == 0) {
                    //当前硬盘没处理任务，map中也无任务，则结束
                    printf("#\n");
                    break;
                }
                all_disk_map_erase(current_request2[i]);
                if (get_distance2(i) <= G) {
                    //若当前磁头位置距离对象存储的首地址<=G,则以p+r的方式读
                    int dis = get_distance2(i);
                    if (dis <= 8) {
                        pAss2[i] = 0;
                        rEad2[i] = object[current_object_id2[i]].size + dis;
                    }
                    else {
                        pAss2[i] = dis;
                        rEad2[i] = object[current_object_id2[i]].size;
                    }
                }
                else {
                    //以j+r的方式读
                    jUmp2[i] = 1;
                    rEad2[i] = object[current_object_id2[i]].size;
                }
            }
            else {//当前磁盘在处理任务
                //先处理j行为
                if (jUmp2[i] > 0) {
                    if (current_token2[i] == 0) {
                        current_token2[i] = G;
                        current_disk_pos2[i] = current_first_pos2[i];
                        current_haveread2[i] = 0;
                        printf("j %d\n", current_first_pos2[i]);
                        jUmp2[i]--;
                        break;
                    }
                    else {
                        printf("#\n");
                        break;
                    }
                }
                //再处理p行为
                else if (pAss2[i] > 0) {
                    if (current_token2[i] < G) {
                        current_token2[i] += 1;
                        current_disk_pos2[i] += 1;
                        current_disk_pos2[i] = (current_disk_pos2[i] - 1) % V + 1;
                        current_haveread2[i] = 0;
                        printf("p");
                        pAss2[i]--;
                    }
                    else {
                        printf("#\n");
                        break;
                    }
                }
                //再处理r行为
                else {
                    if (rEad2[i] > 0) {
                        if (G - current_token2[i] >= read_resume(current_haveread2[i])) {
                            current_token2[i] += read_resume(current_haveread2[i]);
                            current_disk_pos2[i] += 1;
                            current_disk_pos2[i] = (current_disk_pos2[i] - 1) % V + 1;
                            current_haveread2[i] += 1;
                            printf("r");
                            rEad2[i]--;
                        }
                        else {
                            printf("#\n");
                            break;
                        }
                    }
                    else { // 任务完成
                        if (!object[current_object_id2[i]].is_delete) {
                            int cur = current_request2[i];
                            while (cur != 0) {
                                if (!request[cur].is_done && !request[cur].is_busy) {
                                    request[cur].is_done = true;
                                    nums++;
                                    request_ans.push(cur);
                                    cur = request[cur].prev_id;
                                }
                                else {
                                    break;
                                }
                            }
                        }
                        current_request2[i] = 0; // 重置任务状态
                        current_object_id2[i] = 0;
                        current_replica2[i] = 0;
                        current_first_pos2[i] = 0;
                    }
                }
            }
        }

        if (i == N) {
            if (nums == 0) {
                printf("0\n");
            }
            else {
                printf("%d\n", nums);
                for (int i = 1; i <= nums; i++) {
                    printf("%d\n", request_ans.front());
                    request_ans.pop();
                }
            }
        }
    }

    if (t > 105) {
        for (auto x : time_to_request[t - 105]) {
            if (!request[x].is_done && !request[x].is_busy && !object[request[x].object_id].is_delete) {
                request_busy_ans.push(x);
                request[x].is_busy = true;
            }
        }
    }

    if (request_busy_ans.size() == 0) {
        printf("0\n");
    }
    else {
        printf("%d\n", request_busy_ans.size());
        while (!request_busy_ans.empty()) {
            printf("%d\n", request_busy_ans.front());
            request_busy_ans.pop();
        }
    }
    
    fflush(stdout);
}

void clean()
{
    for (auto& obj : object) {
        for (int i = 1; i <= REP_NUM; i++) {
            if (obj.unit[i] == nullptr)
                continue;
            free(obj.unit[i]);
            obj.unit[i] = nullptr;
        }
    }
}

void gc_action()
{
    scanf("%*s %*s");
    printf("GARBAGE COLLECTION\n");
    for (int i = 1; i <= N; i++) {
        printf("0\n");
    }
    fflush(stdout);
}

int main()
{


    scanf("%d%d%d%d%d%d", &T, &M, &N, &V, &G, &K);

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int j = 1; j <= (T + 105 - 1) / FRE_PER_SLICING + 1; j++) {
        scanf("%*d");
    }

    printf("OK\n");
    fflush(stdout);

    for (int i = 1; i <= N; i++) {
        disk_point[i] = 1;
    }

    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        timestamp_action();
        delete_action();
        write_action();
        read_action(t);
        if (t % FRE_PER_SLICING == 0) {
            gc_action();
        }
    }
    clean();

    return 0;
}