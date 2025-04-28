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
    int* unit[REP_NUM + 1];//unit[i]��һ����ά���飬unit[i][j]�ǵ�i�������ĵ�j���λ��
    int size;
    int category;//��������
    int last_request_point;
    bool is_delete;
    bool is_continuous[REP_NUM + 1];//���󸱱��Ƿ������洢��Ӳ����
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
    if (replica_index == 1) {//�����1�Ÿ�������ǰ����д
        for (int i = 1; i <= V; i++) {
            if (disk_unit[i] == 0) {  // ���п�
                if (count == 0) start = i;  // ��¼������ʼλ��
                count++;
                if (count == size) {  // �ҵ��㹻�����ռ�
                    find = true;
                    break;
                }
            }
            else {  // �����ǿտ飬���ü���
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
    else {//�����2�Ż�3�Ÿ������Ӻ���ǰд
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
            is_continuous = false;//�������ĺ��򲿷�λ����ǰ�򲿷�λ�ò�������������Ϊ������
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
            object[id].unit[j] = static_cast<int*>(malloc(sizeof(int) * (size + 1)));//��j������д���Ǹ������ϵ��ĸ��ط�
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
vector<unordered_map<int,int>> disk_request_map(MAX_DISK_NUM); //����map

vector<int> current_request1   (MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ����������
vector<int> current_object_id1 (MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ����Ķ���
vector<int> current_replica1  (MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ�����Ƕ���ļ��Ÿ���
vector<int> current_first_pos1 (MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ���������׵�ַ

vector<int> current_haveread1  (MAX_DISK_NUM); //��¼ÿ��Ӳ���Ѿ��������˶��ٴζ�����
vector<int> current_disk_pos1(MAX_DISK_NUM, 1); //��¼ÿ��Ӳ�̵�ǰ��ͷλ�ã���ʼ��ʱΪ1
vector<int> pAss1(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫpass�Ĵ���
vector<int> jUmp1(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫjump�Ĵ���
vector<int> rEad1(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫread�Ĵ���

vector<int> current_request2(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ����������
vector<int> current_object_id2(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ����Ķ���
vector<int> current_replica2(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ�����Ƕ���ļ��Ÿ���
vector<int> current_first_pos2(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ���������׵�ַ

vector<int> current_haveread2(MAX_DISK_NUM); //��¼ÿ��Ӳ���Ѿ��������˶��ٴζ�����
vector<int> current_disk_pos2(MAX_DISK_NUM, 1); //��¼ÿ��Ӳ�̵�ǰ��ͷλ�ã���ʼ��ʱΪ1
vector<int> pAss2(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫpass�Ĵ���
vector<int> jUmp2(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫjump�Ĵ���
vector<int> rEad2(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫread�Ĵ���

//��һ������������Ѿ������map��ɾ��
void all_disk_map_erase(int erase_request_id) {
    int erase_object_id = request[erase_request_id].object_id;    // �ҵ�Ҫ��������Ķ����
    for (int i = 1; i <= 3; i++) {
        if (object[erase_object_id].is_continuous[i]) {                 //���i�Ÿ����������洢��
            int erase_disk_id = object[erase_object_id].replica[i];     // �ҵ�i�Ÿ����Ĵ洢Ӳ�̺�
            int first_pos = object[erase_object_id].unit[i][1];         //�ҵ�i�Ÿ����׵�ַ
            disk_request_map[erase_disk_id].erase(first_pos);           //ɾ��
        }
    }
}


//�ӵ�i��Ӳ���л�ȡһ����������,��ǰ�߼�Ϊ�����ͷ�����
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
        current_request1[i] = 0; // ��������״̬
        current_object_id1[i] = 0;
        current_replica1[i] = 0;
        current_first_pos1[i] = 0;
        return;
    }

    
    current_request1[i]   = return_request;                       //���¸ô��̵�ǰ����������  
    current_object_id1[i] = request[current_request1[i]].object_id;//���¸ô��̵�ǰ����������
    for (int j = 1; j <= 3; j++) {
        if (object[current_object_id1[i]].replica[j] == i) {
            current_replica1[i] = j;                              //���¸ô��̵�ǰ��������񸱱���
            break;
        }
    }
    current_first_pos1[i] = object[current_object_id1[i]].unit[current_replica1[i]][1];//���¸ô��̵�ǰ����������׵�ַ
    
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
        current_request2[i] = 0; // ��������״̬
        current_object_id2[i] = 0;
        current_replica2[i] = 0;
        current_first_pos2[i] = 0;
        return;
    }


    current_request2[i] = return_request;                       //���¸ô��̵�ǰ����������  
    current_object_id2[i] = request[current_request2[i]].object_id;//���¸ô��̵�ǰ����������
    for (int j = 1; j <= 3; j++) {
        if (object[current_object_id2[i]].replica[j] == i) {
            current_replica2[i] = j;                              //���¸ô��̵�ǰ��������񸱱���
            break;
        }
    }
    current_first_pos2[i] = object[current_object_id2[i]].unit[current_replica2[i]][1];//���¸ô��̵�ǰ����������׵�ַ

    return;
}

//���Ѿ������Ĵ���ת��Ϊ��һ�ζ�����������
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
    //int pos = (current_disk_pos1[i] - 1) % V + 1; // ȷ��λ����[1, V]
    int pos = current_disk_pos1[i];
    int target = current_first_pos1[i];
    if (pos <= target) return target - pos;
    else return (V - pos) + target;
}

int get_distance2(int i) {
    //int pos = (current_disk_pos2[i] - 1) % V + 1; // ȷ��λ����[1, V]
    int pos = current_disk_pos2[i];
    int target = current_first_pos2[i];
    if (pos <= target) return target - pos;
    else return (V - pos) + target;
}

bool not_busy(int first_pos,int i) {
    int legth = V/6;
    bool d1 = true;
    bool d2 = true;
    //int pos1 = (current_disk_pos1[i] - 1) % V + 1; // ȷ��λ����[1, V]
    //int pos2 = (current_disk_pos2[i] - 1) % V + 1; // ȷ��λ����[1, V]
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
    //ÿ��ʱ�䲽�������õ���Ŀ
    vector<int> current_token1(MAX_DISK_NUM,0);//��¼ÿ��Ӳ�̵�ǰʱ�䲽���ĵ�������
    vector<int> current_token2(MAX_DISK_NUM, 0);//��¼ÿ��Ӳ�̵�ǰʱ�䲽���ĵ�������
    int nums = 0;               //��¼��ǰʱ�䲽��ɵ��������
    queue<int> request_ans;   //��¼��ǰʱ�䲽��ɵ�����

    queue<int> request_busy_ans;   //��¼��ǰʱ�䲽busy������

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

        //������������������������������map��
        if (object[object_id].is_continuous[1]&& not_busy(object[object_id].unit[1][1], object[object_id].replica[1])) { //���������������1�Ÿ������������������������map��
            int disk_index = object[object_id].replica[1];        //�ҵ��洢��ǰ����1�Ÿ�����Ӳ�̺�
            int first_pos = object[object_id].unit[1][1];         //�ҵ���ǰ�����׵�ַ
            disk_request_map[disk_index][first_pos] = request_id; //�ѵ�ǰ�����������ӦӲ�̵�map��
        }
        else {
            request_busy_ans.push(request_id);
            request[request_id].is_busy = true;
        }
    }
    
    for (int i = 1; i <= N; i++) {
        
        while (1) {
            if (current_request1[i] == 0) {             //����ǰӲ��δ��������
                get_mission1(i,t);
                if (current_request1[i] == 0) {
                    //��ǰӲ��û��������map��Ҳ�����������
                    printf("#\n");
                    break;
                }
                all_disk_map_erase(current_request1[i]);
                if (get_distance1(i) <= G) {
                    //����ǰ��ͷλ�þ������洢���׵�ַ<=G,����p+r�ķ�ʽ��
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
                    //��j+r�ķ�ʽ��
                    jUmp1[i] = 1;
                    rEad1[i] = object[current_object_id1[i]].size;
                }
            }
            else {//��ǰ�����ڴ�������
                //�ȴ���j��Ϊ
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
                //�ٴ���p��Ϊ
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
                //�ٴ���r��Ϊ
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
                    else { // �������
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
                        current_request1[i] = 0; // ��������״̬
                        current_object_id1[i] = 0;
                        current_replica1[i] = 0;
                        current_first_pos1[i] = 0;
                    }
                }
            }
        }

        while (1) {
            if (current_request2[i] == 0) {             //����ǰӲ��δ��������
                get_mission2(i,t);
                if (current_request2[i] == 0) {
                    //��ǰӲ��û��������map��Ҳ�����������
                    printf("#\n");
                    break;
                }
                all_disk_map_erase(current_request2[i]);
                if (get_distance2(i) <= G) {
                    //����ǰ��ͷλ�þ������洢���׵�ַ<=G,����p+r�ķ�ʽ��
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
                    //��j+r�ķ�ʽ��
                    jUmp2[i] = 1;
                    rEad2[i] = object[current_object_id2[i]].size;
                }
            }
            else {//��ǰ�����ڴ�������
                //�ȴ���j��Ϊ
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
                //�ٴ���p��Ϊ
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
                //�ٴ���r��Ϊ
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
                    else { // �������
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
                        current_request2[i] = 0; // ��������״̬
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