#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <stack>
#include <queue>
#include <vector>
#include <climits>
#include <map>
#include <set>
#include<unordered_set>
#include <algorithm>
#include<unordered_map>
using namespace std;

// 定义常量，使用宏定义以提高编译时效率
#define MAX_DISK_NUM (10 + 1)        // 最大磁盘数 + 1 (1-based indexing)
#define MAX_DISK_SIZE (16384 + 1)    // 最大磁盘大小 + 1
#define MAX_REQUEST_NUM (30000000 + 1) // 最大请求数 + 1
#define MAX_OBJECT_NUM (100000 + 1)  // 最大对象数 + 1
#define REP_NUM (3)                  // 每个对象的副本数
#define FRE_PER_SLICING (1800)       // 每 1800 个时间片进行一次切片
#define EXTRA_TIME (105)             // 额外时间

// 定义 Request 结构体，表示一个请求
typedef struct Request_ {
    int object_id;    // 请求关联的对象 ID
    int prev_id;      // 上一个请求的 ID，形成请求链表
    bool is_done;     // 请求是否已完成
    int born_time;
} Request;

// 定义 Object 结构体，表示一个存储对象
typedef struct Object_ {
    int replica[REP_NUM + 1];         // 对象的副本所在的磁盘 ID (1-based)
    int* unit[REP_NUM + 1];           // 对象的副本存储单元位置，动态分配的数组，unit[i][j] 表示第 i 个副本的第 j 个位置
    int size;                         // 对象大小（占用多少磁盘单元）
    int category;                     // 对象类别
    int last_request_point;           // 指向该对象的最新请求 ID（请求链表头）
    bool is_delete;                   // 对象是否被删除
    bool is_continuous[REP_NUM + 1];  // 每个副本是否连续存储在磁盘上
} Object;

// 全局变量
Request request[MAX_REQUEST_NUM];     // 请求数组，静态分配
Object object[MAX_OBJECT_NUM];        // 对象数组，静态分配
int T, M, N, V, G;                    // T: 时间步数, M: 类别数, N: 磁盘数, V: 磁盘容量, G: 单步最大操作时间
int disk[MAX_DISK_NUM][MAX_DISK_SIZE]; // 磁盘存储状态，disk[i][j] 表示第 i 个磁盘的第 j 个位置存储的对象 ID
int disk_point[MAX_DISK_NUM];         // 磁盘写入指针（未使用，可能是遗留代码）


vector<vector<int>> read_ops;
vector<vector<int>> write_ops;
vector<vector<int>> delete_ops;

vector<vector<int>> tag_region_start;       // 每个标签的起始区域编号（从1开始）
vector<vector<int>> tag_region_end;         // 每个标签的终止区域编号（从1开始）

int tag_first_write[MAX_DISK_NUM][17];  // 假设MAX_DISK_NUM足够大，tag为M
vector<int> total_used(MAX_DISK_NUM, 0);         // 磁盘使用量
static std::map<int, int> last_used_disk_index;   // 轮询索引（按标签）

vector<vector<int>> disk_main_tags;  // 每个磁盘维护的主标签集合
vector<vector<int>> tag_main_disks; // 每个标签的两个主磁盘
vector<int> replica_region_start;          // 副区域起始位置
vector<int> replica_region_end;            // 副区域结束位置
vector<vector<int>> tag_region_point; // 每个标签区域的当前写入位置



// 处理时间戳输入并输出
void timestamp_action() {
    int timestamp;
    scanf("%*s%d", &timestamp);    // 读取 "TIMESTAMP X"，忽略 "TIMESTAMP"，读取时间戳 X
    printf("TIMESTAMP %d\n", timestamp); // 输出时间戳
    fflush(stdout);                // 刷新输出缓冲区，确保输出立即显示
}

// 删除对象的存储单元，将磁盘上的对应位置清零
void do_object_delete(const int* object_unit, int* disk_unit, int size) {
    for (int i = 1; i <= size; i++) {
        disk_unit[object_unit[i]] = 0; // 将对象占用的磁盘位置清零
    }
}

// 处理删除操作
void delete_action() {
    int n_delete;                  // 要删除的对象数量
    int abort_num = 0;             // 未完成请求的数量
    static int _id[MAX_OBJECT_NUM]; // 静态数组，存储待删除的对象 ID，避免重复分配

    scanf("%d", &n_delete);        // 读取删除操作的数量
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);      // 读取每个待删除的对象 ID
    }

    // 第一遍：统计未完成的请求数量
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point; // 从对象的请求链表头开始
        while (current_id != 0) {    // 遍历请求链表
            if (!request[current_id].is_done) { // 如果请求未完成
                abort_num++;
            }
            current_id = request[current_id].prev_id; // 移动到上一个请求
        }
    }

    printf("%d\n", abort_num);    // 输出未完成请求的数量

    // 第二遍：输出未完成请求 ID 并执行删除
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {    // 遍历请求链表
            if (!request[current_id].is_done) { // 如果请求未完成
                printf("%d\n", current_id); // 输出请求 ID
            }
            current_id = request[current_id].prev_id;
        }
        for (int j = 1; j <= REP_NUM; j++) { // 遍历对象的每个副本
            total_used[object[id].replica[j]] -= object[id].size;
            do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size); // 删除副本
        }
        object[id].is_delete = true; // 标记对象为已删除
    }

    fflush(stdout);                // 刷新输出缓冲区
}


// 将对象写入磁盘的指定位置
void do_object_write(int* object_unit, int* disk_unit, int size, int object_id, int replica_index) {
    int d = object[object_id].replica[replica_index];  // 获取该副本对应的磁盘编号
    int tag = object[object_id].category;              // 获取该对象所属的标签（类别）
    int start = -1;                                     // 起始写入位置初始化
    bool found = false;                                 // 是否找到合适的连续空间
    object[object_id].is_continuous[replica_index] = true;  // 默认标记为连续存储

    // 第一步：如果该磁盘是该标签的主磁盘，则优先在主区域写入
    if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) != disk_main_tags[d].end()) {
        int region_start = tag_region_point[d][tag];  // 获取主区域起始位置
        int region_end = tag_region_end[d][tag];      // 获取主区域结束位置
        // 如果是第一次写入，并且主区域内还有足够空间
        if (tag_first_write[d][tag] == 1 && region_start + size - 1 <= region_end) {
            int count = 0;
            // 在主区域内查找一段连续的空闲空间
            for (int i = region_start; i <= region_end; i++) {
                if (disk_unit[i] == 0) {  // 空闲块
                    if (count == 0) start = i;  // 记录连续开始位置
                    count++;
                    if (count == size) {  // 找到足够连续空间
                        found = true;
                        tag_region_point[d][tag] += size;  // 更新区域起始指针
                        break;
                    }
                }
                else {  // 遇到非空块，重置计数
                    count = 0;
                    start = -1;
                }
            }
            // 如果主区域被写满或找不到空间，则关闭首次写入标记
            if (found && start + size - 1 >= region_end) {
                tag_first_write[d][tag] = 0;
            }
            else if (!found) {
                tag_first_write[d][tag] = 0;
            }
        }
        // 第二步：如果主区域写失败，则尝试在副区域中写入
        if (tag_first_write[d][tag] == 0&& !found) {
            for (int t = 1; t <= M; t++) {
                if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) != disk_main_tags[d].end())continue;
                int rs = tag_region_point[d][t];
                int re = tag_region_end[d][t];
                int count = 0;
                if (tag_first_write[d][t] == 1) {
                    for (int i = rs; i <= re; i++) {
                        if (disk_unit[i] == 0) {
                            if (count == 0) start = i;
                            count++;
                            if (count == size && tag_region_point[d][t] + size <= tag_region_end[d][t]) {
                                tag_region_point[d][t] += size;
                                found = true;
                                break;
                            }
                        }
                        else {
                            count = 0;
                            start = -1;
                        }
                    }
                }
                if (found) break;
            }
        }
        if (!found) {
            int count = 0;
            for (int i = tag_region_start[d][tag]; i <= region_end; i++) {
                if (disk_unit[i] == 0) {
                    if (count == 0) start = i;
                    count++;
                    if (count == size) {
                        found = true;
                        break;
                    }
                }
                else {
                    count = 0;
                    start = -1;
                }
            }
        }


    }

    if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) == disk_main_tags[d].end()) {
        int region_start = tag_region_point[d][tag];  // 获取副区域起始位置
        int region_end = tag_region_end[d][tag];      // 获取副区域结束位置
        if (tag_first_write[d][tag] == 1 && region_start + size - 1 <= region_end) {
            int count = 0;
            // 在主区域内查找一段连续的空闲空间
            for (int i = region_start; i <= region_end; i++) {
                if (disk_unit[i] == 0) {  // 空闲块
                    if (count == 0) start = i;  // 记录连续开始位置
                    count++;
                    if (count == size) {  // 找到足够连续空间
                        found = true;
                        tag_region_point[d][tag] += size;  // 更新区域起始指针
                        break;
                    }
                }
                else {  // 遇到非空块，重置计数
                    count = 0;
                    start = -1;
                }
            }
            // 如果主区域被写满或找不到空间，则关闭首次写入标记
            if (found && start + size - 1 >= region_end) {
                tag_first_write[d][tag] = 0;
            }
            else if (!found) {
                tag_first_write[d][tag] = 0;
            }
        }

        if (tag_first_write[d][tag] == 0 && !found) {
            for (int t = 1; t <= M; t++) {
                if (t == tag)continue;
                if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), t) != disk_main_tags[d].end())continue;
                int rs = tag_region_point[d][t];
                int re = tag_region_end[d][t];
                int count = 0;
                if (tag_first_write[d][t] == 1) {
                    for (int i = rs; i <= re; i++) {
                        if (disk_unit[i] == 0) {
                            if (count == 0) start = i;
                            count++;
                            if (count == size && tag_region_point[d][t] + size <= tag_region_end[d][t]) {
                                tag_region_point[d][t] += size;
                                found = true;
                                break;
                            }
                        }
                        else {
                            count = 0;
                            start = -1;
                        }
                    }
                }
                if (found) break;
            }
            if (!found) {
                
                int count = 0;
                for (int i = replica_region_start[d]; i <= V; i++) {
                    if (disk_unit[i] == 0) {
                        if (count == 0) start = i;
                        count++;
                        if (count == size) {
                            found = true;
                            break;
                        }
                    }
                    else {
                        count = 0;
                        start = -1;
                    }
                }
            }
            if (!found) {
                for (auto tagfather : disk_main_tags[d]) {
                    int region_start = tag_region_point[d][tagfather];  // 获取副区域起始位置
                    int region_end = tag_region_end[d][tagfather];      // 获取副区域结束位置
                    if (tag_first_write[d][tagfather] == 1 && region_start + size - 1 <= region_end) {
                        int count = 0;
                        // 在主区域内查找一段连续的空闲空间
                        for (int i = region_start; i <= region_end; i++) {
                            if (disk_unit[i] == 0) {  // 空闲块
                                if (count == 0) start = i;  // 记录连续开始位置
                                count++;
                                if (count == size) {  // 找到足够连续空间
                                    found = true;
                                    tag_region_point[d][tagfather] += size;  // 更新区域起始指针
                                    break;
                                }
                            }
                            else {  // 遇到非空块，重置计数
                                count = 0;
                                start = -1;
                            }
                        }
                        // 如果主区域被写满或找不到空间，则关闭首次写入标记
                        if (found && start + size - 1 >= region_end) {
                            tag_first_write[d][tagfather] = 0;
                        }
                        else if (!found) {
                            tag_first_write[d][tagfather] = 0;
                        }
                    }
                }
            }
        }
    }
   

    // 第三步：如果主区域和副区域都失败，则遍历整个磁盘进行写入
    if (!found) {
        int count = 0;
        for (int i = 1; i <= V; i++) {
            if (disk_unit[i] == 0) {
                if (count == 0) start = i;
                count++;
                if (count == size) {
                    found = true;
                    break;
                }
            }
            else {
                count = 0;
                start = -1;
            }
        }
    }

    // 如果找到了连续空间，则写入数据
    if (found) {
        for (int i = 0; i < size; i++) {
            disk_unit[start + i] = object_id;           // 写入对象ID
            object_unit[i + 1] = start + i;              // 记录位置
        }
    }
    else {
        // 如果未找到连续空间，则采用非连续写入
        int current_write_point = 0;
        for (int i = 1; i <= V; i++) {
            if (disk_unit[i] == 0) {
                disk_unit[i] = object_id;
                object_unit[++current_write_point] = i;
                if (current_write_point == size) break;
            }
        }
        object[object_id].is_continuous[replica_index] = false;  // 标记为非连续存储
    }

    total_used[d] += size;  // 更新该磁盘的使用量
}


// 处理写入操作
void write_action() {
    int n_write;
    scanf("%d", &n_write);  // 读取写入请求的数量
    for (int i = 1; i <= n_write; i++) {
        int id, size, category;
        scanf("%d%d%d", &id, &size, &category);  // 读取对象ID、大小和类别
        object[id].last_request_point = 0;  // 将请求链表的头置为0
        object[id].category = category;  // 设置对象的类别

        int total_disk_used = 0;
        for (int d = 1; d <= N; d++) {
      
            total_disk_used += total_used[d];
        }
        

        vector<int> selected_disks;  // 用于存储选择的磁盘

        // 第一步：将前两个副本分配给主要磁盘
        for (int j = 0; j < 2; j++) {
            int main_disk = tag_main_disks[category][j];  // 获取当前标签的主要磁盘
            if (total_used[main_disk] + size <= 0.95*V&& find(selected_disks.begin(), selected_disks.end(), main_disk) == selected_disks.end()) {  // 如果磁盘空间足够
                selected_disks.push_back(main_disk);  // 将磁盘加入选择列表
            }
            else {
                // 如果空间不足，尝试找到一个备选磁盘
                for (int d = 1; d <= N; d++) {
                    // 检查当前磁盘是否未被使用并且空间足够
                    if (total_used[d] + size <= 0.9*V &&
                        find(selected_disks.begin(), selected_disks.end(), d) == selected_disks.end()) {
                        selected_disks.push_back(d);  // 将备选磁盘加入选择列表
                        break;
                    }
                }
            }
        }

        // 第二步：在副本区域使用轮询机制为第三个副本分配磁盘
        int& current_index1 = last_used_disk_index[category];  // 获取当前标签的磁盘轮询索引
        current_index1++;
        for (int attempts = 0; attempts < N; attempts++) {
            int current_index = (current_index1 % N) + 1;  // 轮询下一个磁盘
            int candidate = current_index;
            // 如果该磁盘未被使用，并且空间足够，则选择该磁盘
            if (find(selected_disks.begin(), selected_disks.end(), candidate) == selected_disks.end() &&
                total_used[candidate] + size < 0.95*V&& (candidate!=7|| candidate != 9)) {
                selected_disks.push_back(candidate);  // 将该磁盘加入选择列表
                break;
            }
        }

        // 回退机制：如果找不到合适的磁盘，强制选择一个未使用的磁盘
        if (selected_disks.size() < 3) {
            for (int d = N; d >=1; d--) {
                if (find(selected_disks.begin(), selected_disks.end(), d) == selected_disks.end() &&
                    total_used[d] + size <= 0.95*V) {
                    selected_disks.push_back(d);  // 将磁盘加入选择列表
                    if (selected_disks.size() == 3) break;
                }
            }
        }

        // 第三步：为每个副本执行写入操作
        for (int j = 1; j <= REP_NUM; j++) {
            int disk_id = selected_disks[j - 1];  // 获取当前副本的磁盘ID
            object[id].replica[j] = disk_id;  // 设置副本所在磁盘ID
            object[id].unit[j] = static_cast<int*>(malloc(sizeof(int) * (size + 1)));  // 为副本分配存储空间
            object[id].size = size;  // 设置对象大小
            object[id].is_delete = false;  // 标记对象未删除
            do_object_write(object[id].unit[j], disk[disk_id], size, id, j);  // 执行写入操作
        }

        printf("%d\n", id);  // 输出对象ID
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", object[id].replica[j]);  // 输出副本所在的磁盘ID
            for (int k = 1; k <= size; k++) {
                printf(" %d", object[id].unit[j][k]);  // 输出副本存储的位置信息
            }
            printf("\n");
        }
    }
    fflush(stdout);  // 刷新输出缓冲区，确保输出立即显示
}


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
vector<unordered_map<int, int>> disk_request_map(MAX_DISK_NUM); //任务map

vector<int> current_request(MAX_DISK_NUM); //记录每个硬盘当前处理的任务号
vector<int> current_object_id(MAX_DISK_NUM); //记录每个硬盘当前处理的对象
vector<int> current_replica(MAX_DISK_NUM); //记录每个硬盘当前处理是对象的几号副本
vector<int> current_first_pos(MAX_DISK_NUM); //记录每个硬盘当前处理对象的首地址

vector<int> current_haveread(MAX_DISK_NUM); //记录每个硬盘已经连续做了多少次读操作
vector<int> current_disk_pos(MAX_DISK_NUM, 1); //记录每个硬盘当前磁头位置，初始化时为1
vector<int> pAss(MAX_DISK_NUM);               //记录每个硬盘当前需要pass的次数
vector<int> jUmp(MAX_DISK_NUM);               //记录每个硬盘当前需要jump的次数
vector<int> rEad(MAX_DISK_NUM);               //记录每个硬盘当前需要read的次数



//把一个请求从所有已经进入的set中删除
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
void get_mission(int i, int current_time) {

    int return_request = 0;
    for (int j = 0; j < V; j++) {
        int k = (j + current_disk_pos[i] - 1) % V + 1;
        if (disk_request_map[i].find(k) != disk_request_map[i].end() && disk_request_map[i][k] != 0) {
            int req_id = disk_request_map[i][k];
            if (object[request[disk_request_map[i][k]].object_id].is_delete || (current_time - request[req_id].born_time > 70)) {
                disk_request_map[i][k] = 0;
                continue;
            }
            return_request = req_id;
            break;
        }
    }

    if (return_request == 0) {
        current_request[i] = 0; // 重置任务状态
        current_object_id[i] = 0;
        current_replica[i] = 0;
        current_first_pos[i] = 0;
        return;
    }


    current_request[i] = return_request;                       //更新该磁盘当前处理的请求号  
    current_object_id[i] = request[current_request[i]].object_id;//更新该磁盘当前处理的任务号
    for (int j = 1; j <= 3; j++) {
        if (object[current_object_id[i]].replica[j] == i) {
            current_replica[i] = j;                              //更新该磁盘当前处理的任务副本号
            break;
        }
    }
    current_first_pos[i] = object[current_object_id[i]].unit[current_replica[i]][1];//更新该磁盘当前处理的任务首地址

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

int get_distance(int i) {
    int pos = (current_disk_pos[i] - 1) % V + 1; // 确保位置在[1, V]
    int target = current_first_pos[i];
    if (pos <= target) return target - pos;
    else return (V - pos) + target;
}

void read_action(int t)
{
    int n_read;
    int request_id, object_id;
    //每个时间步都会重置的项目
    vector<int> current_token(MAX_DISK_NUM, 0);//记录每个硬盘当前时间步消耗的令牌数
    int nums = 0;               //记录当前时间步完成的请求个数
    queue<int> request_ans;   //记录当前时间步完成的请求

    scanf("%d", &n_read);

    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
        request[request_id].born_time = t; // 记录请求的生成时间片
        //如果该任务是连续储存的则让他进入map中
        for (int j = 1; j <= 3; j++) {
            if (object[object_id].is_continuous[j]) {                 //如果该副本是连续储存的则让他进入栈中
                int disk_index = object[object_id].replica[j];        //找到存储当前任务j号副本的硬盘号
                int first_pos = object[object_id].unit[j][1];         //找到当前副本首地址
                disk_request_map[disk_index][first_pos] = request_id; //把当前任务号塞进对应硬盘的map内
            }
        }
    }

    for (int i = 1; i <= N; i++) {
        while (1) {

            if (current_request[i] == 0) {             //若当前硬盘未处理任务
                get_mission(i,t);
                if (current_request[i] == 0) {
                    //当前硬盘没处理任务，map中也无任务，则结束
                    printf("#\n");
                    break;
                }
                all_disk_map_erase(current_request[i]);
                if (get_distance(i) <= G) {
                    //若当前磁头位置距离对象存储的首地址<=G,则以p+r的方式读
                    pAss[i] = get_distance(i);
                    rEad[i] = object[current_object_id[i]].size;
                }
                else {
                    //以j+r的方式读
                    jUmp[i] = 1;
                    rEad[i] = object[current_object_id[i]].size;
                }
            }
            else {//当前磁盘在处理任务
                //先处理j行为
                if (jUmp[i] > 0) {
                    if (current_token[i] == 0) {
                        current_token[i] = G;
                        current_disk_pos[i] = current_first_pos[i];
                        current_haveread[i] = 0;
                        printf("j %d\n", current_first_pos[i]);
                        jUmp[i]--;
                        break;
                    }
                    else {
                        printf("#\n");
                        break;
                    }
                }
                //再处理p行为
                else if (pAss[i] > 0) {
                    if (current_token[i] < G) {
                        current_token[i] += 1;
                        current_disk_pos[i] += 1;
                        current_haveread[i] = 0;
                        printf("p");
                        pAss[i]--;
                    }
                    else {
                        printf("#\n");
                        break;
                    }
                }
                //再处理r行为
                else {
                    if (rEad[i] > 0) {
                        if (G - current_token[i] >= read_resume(current_haveread[i])) {
                            current_token[i] += read_resume(current_haveread[i]);
                            current_disk_pos[i] += 1;
                            current_haveread[i] += 1;
                            printf("r");
                            rEad[i]--;
                        }
                        else {
                            printf("#\n");
                            break;
                        }
                    }
                    else { // 任务完成
                        if (!object[current_object_id[i]].is_delete) {
                            int cur = current_request[i];
                            while (cur != 0) {
                                if (!request[cur].is_done) {
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
                        current_request[i] = 0; // 重置任务状态
                        current_object_id[i] = 0;
                        current_replica[i] = 0;
                        current_first_pos[i] = 0;
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

    fflush(stdout);
}

// 清理动态分配的内存
void clean() {
    for (int i = 0; i < MAX_OBJECT_NUM; i++) { // 遍历所有对象
        for (int j = 1; j <= REP_NUM; j++) {
            if (object[i].unit[j] != nullptr) { // 如果内存已分配
                free(object[i].unit[j]); // 释放内存
                object[i].unit[j] = nullptr; // 置空指针
            }
        }
    }
}

int main() {
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    int slices = (T + FRE_PER_SLICING - 1) / FRE_PER_SLICING;

    // 读取预处理数据
    vector<vector<int>> fre_del(M + 1, vector<int>(slices + 1));
    vector<vector<int>> fre_write(M + 1, vector<int>(slices + 1));
    vector<vector<int>> fre_read(M + 1, vector<int>(slices + 1));

    // 读取删除量
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slices; j++) {
            scanf("%d", &fre_del[i][j]);
        }
    }

    // 读取写入量
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slices; j++) {
            scanf("%d", &fre_write[i][j]);
        }
    }

    // 读取读取量（未使用但保留输入）
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slices; j++) {
            scanf("%d", &fre_read[i][j]);
        }
    }

    // 计算每个标签的「写-删最大值」并排序
    vector<pair<int, int>> tag_max_diff_ranks; // {max_diff, tag_id}
    for (int i = 1; i <= M; i++) {
        int max_diff = INT_MIN;
        int diff = 0;
        for (int j = 1; j <= slices; j++) {
            diff += fre_write[i][j] - fre_del[i][j];
            
            if (diff > max_diff) {
                max_diff = diff;
            }
        }
        tag_max_diff_ranks.emplace_back(max_diff, i);
    }

    // 按写-删最大值降序排序
    sort(tag_max_diff_ranks.begin(), tag_max_diff_ranks.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first; // 按最大值降序排列
        });

    // 划分大标签和小标签（前50%和后50%）
    vector<int> big_tags, small_tags;
    int half_m = M / 2;
    for (int i = 0; i < half_m; i++)
        big_tags.push_back(tag_max_diff_ranks[i].second);
    for (int i = half_m; i < M; i++)
        small_tags.push_back(tag_max_diff_ranks[i].second);


    // Initialize data structures
    tag_region_start.resize(N + 1, vector<int>(M + 1));
    tag_region_end.resize(N + 1, vector<int>(M + 1));
    tag_region_point.resize(N + 1, vector<int>(M + 1));
    tag_main_disks.resize(M + 1, vector<int>(2));
    disk_main_tags.resize(N + 1);
    replica_region_start.resize(N + 1);
    replica_region_end.resize(N + 1);

    // Assign main disks to big tags (round-robin from disk 1)
   // Assign main disks to big tags (round-robin from disk 1)
    int big_disk_idx = 1;
    for (int tag : big_tags) {
        // ==== 新增：手动分配标签8和12的磁盘 ====
        if (tag == 8) {
            tag_main_disks[tag][0] = 7;
            tag_main_disks[tag][1] = 8;
            // 将标签8添加到磁盘7和8的主标签列表
            disk_main_tags[7].push_back(tag);
            disk_main_tags[8].push_back(tag);
            continue; // 跳过轮询逻辑
        }
        if (tag == 12) {
            tag_main_disks[tag][0] = 9;
            tag_main_disks[tag][1] = 10;
            // 将标签12添加到磁盘9和10的主标签列表
            disk_main_tags[9].push_back(tag);
            disk_main_tags[10].push_back(tag);
            continue; // 跳过轮询逻辑
        }
        // ==== 原有轮询逻辑（跳过7、8、9、10号磁盘）====
        for (int i = 0; i < 2; i++) {
            // 跳过保留磁盘（7、8、9、10）
            while (big_disk_idx == 7 || big_disk_idx == 8 || big_disk_idx == 9 || big_disk_idx == 10) {
                big_disk_idx = (big_disk_idx % N) + 1;
            }

            tag_main_disks[tag][i] = big_disk_idx;
            if (find(disk_main_tags[big_disk_idx].begin(), disk_main_tags[big_disk_idx].end(), tag)
                == disk_main_tags[big_disk_idx].end()) {
                disk_main_tags[big_disk_idx].push_back(tag);
            }
            big_disk_idx = (big_disk_idx % N) + 1;
        }
    }

    // Assign main disks to small tags (round-robin from where big tags left off)
    int small_disk_idx = big_disk_idx;
    for (int tag : small_tags) {
        for (int i = 0; i < 2; i++) {
            tag_main_disks[tag][i] = small_disk_idx;
            if (find(disk_main_tags[small_disk_idx].begin(), disk_main_tags[small_disk_idx].end(), tag) == disk_main_tags[small_disk_idx].end())
                disk_main_tags[small_disk_idx].push_back(tag);
            small_disk_idx = (small_disk_idx % N) + 1;
        }
    }

    // Allocate disk regions: big tags from top, small tags from bottom, non-main tags in the middle
    for (int d = 1; d <= N; d++) {
        int current_top = 1, current_bottom = V;

        // Allocate big tags' main regions (from top)
        for (int tag : big_tags) {
            if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) == disk_main_tags[d].end())
                continue;
            if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) == disk_main_tags[d].end())
                continue;
            if (tag == 8 || tag == 12) {
                int region_size = max(1, (int)(fre_write[tag][1] * 2.5));
            }
            int region_size = max(1, (int)(fre_write[tag][1] * 1.4));
            if (current_top + region_size - 1 > current_bottom)
                region_size = current_bottom - current_top + 1;

            tag_region_start[d][tag] = current_top;
            tag_region_end[d][tag] = current_top + region_size - 1;
            current_top += region_size;
        }

        // Allocate small tags' main regions (from bottom)
        for (int tag : small_tags) {
            if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) == disk_main_tags[d].end())
                continue;
            int region_size = max(1, (int)(fre_write[tag][1] * 1.2));
            if (current_bottom - region_size + 1 < current_top)
                region_size = current_bottom - current_top + 1;

            if (region_size <= 0) break;

            tag_region_start[d][tag] = current_bottom - region_size + 1;
            tag_region_end[d][tag] = current_bottom;
            current_bottom -= region_size;
        }

        // Allocate non-main tags' regions (remaining middle space)
        int remain_space = current_bottom - current_top + 1;
        vector<int> non_main_tags;
        for (int t = 1; t <= M; t++) {
            int is_main = 0;
            for (auto it : disk_main_tags[d]) {
                if (it == t) is_main = 1;
            }
            if (is_main == 0)
                non_main_tags.push_back(t);
        }
        int num_tags = non_main_tags.size();
        if (num_tags > 0 && remain_space > 0) {
            int base_size = remain_space / num_tags;
            int remainder = remain_space % num_tags;
            int curr = current_top;

            for (int i = 0; i < num_tags; i++) {
                int tag = non_main_tags[i];
                int tag_size = base_size + (i < remainder ? 1 : 0);
                if (tag_size <= 0 || curr > V) break; // Prevent invalid size or overflow
                if (curr + tag_size - 1 > V) {
                    tag_size = V - curr + 1;
                }
                if (tag_size <= 0) break;

                tag_region_start[d][tag] = curr;
                tag_region_end[d][tag] = curr + tag_size - 1;
                curr += tag_size;
            }
        }

        // Set replica region (same as remaining space boundaries)
        replica_region_start[d] = current_top;
        replica_region_end[d] = current_bottom;
    }

    // 调整特定磁盘上标签的起始点和终点
    for (int d = 1; d <= N; d++) {
        if (d == 1 || d == 3 || d == 5) {
            // 交换1号、3号、5号磁盘上两个大标签的区域
            vector<int> big_tags_on_disk;
            for (int tag : big_tags) {
                if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) != disk_main_tags[d].end()) {
                    big_tags_on_disk.push_back(tag);
                }
            }
            if (big_tags_on_disk.size() >= 2) {
                int tag1 = big_tags_on_disk[0]; // 第一个大标签
                int tag2 = big_tags_on_disk[1]; // 第二个大标签
                // 交换起始点和终点
                swap(tag_region_start[d][tag1], tag_region_start[d][tag2]);
                swap(tag_region_end[d][tag1], tag_region_end[d][tag2]);
            }
        }
        else if (d == 7 || d == 9 || d == 1) {
            // 交换7号磁盘上两个小标签的区域
            vector<int> small_tags_on_disk;
            for (int tag : small_tags) {
                if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) != disk_main_tags[d].end()) {
                    small_tags_on_disk.push_back(tag);
                }
            }
            if (small_tags_on_disk.size() >= 2) {
                int tag1 = small_tags_on_disk[0]; // 第一个小标签
                int tag2 = small_tags_on_disk[1]; // 第二个小标签
                // 交换起始点和终点
                swap(tag_region_start[d][tag1], tag_region_start[d][tag2]);
                swap(tag_region_end[d][tag1], tag_region_end[d][tag2]);
            }
        }
    }

    // Initialize write pointers and flags
    for (int d = 1; d <= N; d++) {
        for (int tag = 1; tag <= M; tag++) {
            tag_first_write[d][tag] = 1;
            tag_region_point[d][tag] = tag_region_start[d][tag];
        }
        disk_point[d] = 1;
       if(d==7||d==9)disk_point[d] = 5200;
    }

    printf("OK\n");
    fflush(stdout);

    // Execute time-slice operations
    for (int t = 1; t <= T + EXTRA_TIME; t++) {
        timestamp_action();
        delete_action();
        write_action();
        read_action(t);
    }
    clean();
    return 0;
}