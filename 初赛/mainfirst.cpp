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

// ���峣����ʹ�ú궨������߱���ʱЧ��
#define MAX_DISK_NUM (10 + 1)        // �������� + 1 (1-based indexing)
#define MAX_DISK_SIZE (16384 + 1)    // �����̴�С + 1
#define MAX_REQUEST_NUM (30000000 + 1) // ��������� + 1
#define MAX_OBJECT_NUM (100000 + 1)  // �������� + 1
#define REP_NUM (3)                  // ÿ������ĸ�����
#define FRE_PER_SLICING (1800)       // ÿ 1800 ��ʱ��Ƭ����һ����Ƭ
#define EXTRA_TIME (105)             // ����ʱ��

// ���� Request �ṹ�壬��ʾһ������
typedef struct Request_ {
    int object_id;    // ��������Ķ��� ID
    int prev_id;      // ��һ������� ID���γ���������
    bool is_done;     // �����Ƿ������
    int born_time;
} Request;

// ���� Object �ṹ�壬��ʾһ���洢����
typedef struct Object_ {
    int replica[REP_NUM + 1];         // ����ĸ������ڵĴ��� ID (1-based)
    int* unit[REP_NUM + 1];           // ����ĸ����洢��Ԫλ�ã���̬��������飬unit[i][j] ��ʾ�� i �������ĵ� j ��λ��
    int size;                         // �����С��ռ�ö��ٴ��̵�Ԫ��
    int category;                     // �������
    int last_request_point;           // ָ��ö������������ ID����������ͷ��
    bool is_delete;                   // �����Ƿ�ɾ��
    bool is_continuous[REP_NUM + 1];  // ÿ�������Ƿ������洢�ڴ�����
} Object;

// ȫ�ֱ���
Request request[MAX_REQUEST_NUM];     // �������飬��̬����
Object object[MAX_OBJECT_NUM];        // �������飬��̬����
int T, M, N, V, G;                    // T: ʱ�䲽��, M: �����, N: ������, V: ��������, G: ����������ʱ��
int disk[MAX_DISK_NUM][MAX_DISK_SIZE]; // ���̴洢״̬��disk[i][j] ��ʾ�� i �����̵ĵ� j ��λ�ô洢�Ķ��� ID
int disk_point[MAX_DISK_NUM];         // ����д��ָ�루δʹ�ã��������������룩


vector<vector<int>> read_ops;
vector<vector<int>> write_ops;
vector<vector<int>> delete_ops;

vector<vector<int>> tag_region_start;       // ÿ����ǩ����ʼ�����ţ���1��ʼ��
vector<vector<int>> tag_region_end;         // ÿ����ǩ����ֹ�����ţ���1��ʼ��

int tag_first_write[MAX_DISK_NUM][17];  // ����MAX_DISK_NUM�㹻��tagΪM
vector<int> total_used(MAX_DISK_NUM, 0);         // ����ʹ����
static std::map<int, int> last_used_disk_index;   // ��ѯ����������ǩ��

vector<vector<int>> disk_main_tags;  // ÿ������ά��������ǩ����
vector<vector<int>> tag_main_disks; // ÿ����ǩ������������
vector<int> replica_region_start;          // ��������ʼλ��
vector<int> replica_region_end;            // ���������λ��
vector<vector<int>> tag_region_point; // ÿ����ǩ����ĵ�ǰд��λ��



// ����ʱ������벢���
void timestamp_action() {
    int timestamp;
    scanf("%*s%d", &timestamp);    // ��ȡ "TIMESTAMP X"������ "TIMESTAMP"����ȡʱ��� X
    printf("TIMESTAMP %d\n", timestamp); // ���ʱ���
    fflush(stdout);                // ˢ�������������ȷ�����������ʾ
}

// ɾ������Ĵ洢��Ԫ���������ϵĶ�Ӧλ������
void do_object_delete(const int* object_unit, int* disk_unit, int size) {
    for (int i = 1; i <= size; i++) {
        disk_unit[object_unit[i]] = 0; // ������ռ�õĴ���λ������
    }
}

// ����ɾ������
void delete_action() {
    int n_delete;                  // Ҫɾ���Ķ�������
    int abort_num = 0;             // δ������������
    static int _id[MAX_OBJECT_NUM]; // ��̬���飬�洢��ɾ���Ķ��� ID�������ظ�����

    scanf("%d", &n_delete);        // ��ȡɾ������������
    for (int i = 1; i <= n_delete; i++) {
        scanf("%d", &_id[i]);      // ��ȡÿ����ɾ���Ķ��� ID
    }

    // ��һ�飺ͳ��δ��ɵ���������
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point; // �Ӷ������������ͷ��ʼ
        while (current_id != 0) {    // ������������
            if (!request[current_id].is_done) { // �������δ���
                abort_num++;
            }
            current_id = request[current_id].prev_id; // �ƶ�����һ������
        }
    }

    printf("%d\n", abort_num);    // ���δ������������

    // �ڶ��飺���δ������� ID ��ִ��ɾ��
    for (int i = 1; i <= n_delete; i++) {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0) {    // ������������
            if (!request[current_id].is_done) { // �������δ���
                printf("%d\n", current_id); // ������� ID
            }
            current_id = request[current_id].prev_id;
        }
        for (int j = 1; j <= REP_NUM; j++) { // ���������ÿ������
            total_used[object[id].replica[j]] -= object[id].size;
            do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size); // ɾ������
        }
        object[id].is_delete = true; // ��Ƕ���Ϊ��ɾ��
    }

    fflush(stdout);                // ˢ�����������
}


// ������д����̵�ָ��λ��
void do_object_write(int* object_unit, int* disk_unit, int size, int object_id, int replica_index) {
    int d = object[object_id].replica[replica_index];  // ��ȡ�ø�����Ӧ�Ĵ��̱��
    int tag = object[object_id].category;              // ��ȡ�ö��������ı�ǩ�����
    int start = -1;                                     // ��ʼд��λ�ó�ʼ��
    bool found = false;                                 // �Ƿ��ҵ����ʵ������ռ�
    object[object_id].is_continuous[replica_index] = true;  // Ĭ�ϱ��Ϊ�����洢

    // ��һ��������ô����Ǹñ�ǩ�������̣���������������д��
    if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) != disk_main_tags[d].end()) {
        int region_start = tag_region_point[d][tag];  // ��ȡ��������ʼλ��
        int region_end = tag_region_end[d][tag];      // ��ȡ���������λ��
        // ����ǵ�һ��д�룬�����������ڻ����㹻�ռ�
        if (tag_first_write[d][tag] == 1 && region_start + size - 1 <= region_end) {
            int count = 0;
            // ���������ڲ���һ�������Ŀ��пռ�
            for (int i = region_start; i <= region_end; i++) {
                if (disk_unit[i] == 0) {  // ���п�
                    if (count == 0) start = i;  // ��¼������ʼλ��
                    count++;
                    if (count == size) {  // �ҵ��㹻�����ռ�
                        found = true;
                        tag_region_point[d][tag] += size;  // ����������ʼָ��
                        break;
                    }
                }
                else {  // �����ǿտ飬���ü���
                    count = 0;
                    start = -1;
                }
            }
            // ���������д�����Ҳ����ռ䣬��ر��״�д����
            if (found && start + size - 1 >= region_end) {
                tag_first_write[d][tag] = 0;
            }
            else if (!found) {
                tag_first_write[d][tag] = 0;
            }
        }
        // �ڶ��������������дʧ�ܣ������ڸ�������д��
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
        int region_start = tag_region_point[d][tag];  // ��ȡ��������ʼλ��
        int region_end = tag_region_end[d][tag];      // ��ȡ���������λ��
        if (tag_first_write[d][tag] == 1 && region_start + size - 1 <= region_end) {
            int count = 0;
            // ���������ڲ���һ�������Ŀ��пռ�
            for (int i = region_start; i <= region_end; i++) {
                if (disk_unit[i] == 0) {  // ���п�
                    if (count == 0) start = i;  // ��¼������ʼλ��
                    count++;
                    if (count == size) {  // �ҵ��㹻�����ռ�
                        found = true;
                        tag_region_point[d][tag] += size;  // ����������ʼָ��
                        break;
                    }
                }
                else {  // �����ǿտ飬���ü���
                    count = 0;
                    start = -1;
                }
            }
            // ���������д�����Ҳ����ռ䣬��ر��״�д����
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
                    int region_start = tag_region_point[d][tagfather];  // ��ȡ��������ʼλ��
                    int region_end = tag_region_end[d][tagfather];      // ��ȡ���������λ��
                    if (tag_first_write[d][tagfather] == 1 && region_start + size - 1 <= region_end) {
                        int count = 0;
                        // ���������ڲ���һ�������Ŀ��пռ�
                        for (int i = region_start; i <= region_end; i++) {
                            if (disk_unit[i] == 0) {  // ���п�
                                if (count == 0) start = i;  // ��¼������ʼλ��
                                count++;
                                if (count == size) {  // �ҵ��㹻�����ռ�
                                    found = true;
                                    tag_region_point[d][tagfather] += size;  // ����������ʼָ��
                                    break;
                                }
                            }
                            else {  // �����ǿտ飬���ü���
                                count = 0;
                                start = -1;
                            }
                        }
                        // ���������д�����Ҳ����ռ䣬��ر��״�д����
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
   

    // �����������������͸�����ʧ�ܣ�������������̽���д��
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

    // ����ҵ��������ռ䣬��д������
    if (found) {
        for (int i = 0; i < size; i++) {
            disk_unit[start + i] = object_id;           // д�����ID
            object_unit[i + 1] = start + i;              // ��¼λ��
        }
    }
    else {
        // ���δ�ҵ������ռ䣬����÷�����д��
        int current_write_point = 0;
        for (int i = 1; i <= V; i++) {
            if (disk_unit[i] == 0) {
                disk_unit[i] = object_id;
                object_unit[++current_write_point] = i;
                if (current_write_point == size) break;
            }
        }
        object[object_id].is_continuous[replica_index] = false;  // ���Ϊ�������洢
    }

    total_used[d] += size;  // ���¸ô��̵�ʹ����
}


// ����д�����
void write_action() {
    int n_write;
    scanf("%d", &n_write);  // ��ȡд�����������
    for (int i = 1; i <= n_write; i++) {
        int id, size, category;
        scanf("%d%d%d", &id, &size, &category);  // ��ȡ����ID����С�����
        object[id].last_request_point = 0;  // �����������ͷ��Ϊ0
        object[id].category = category;  // ���ö�������

        int total_disk_used = 0;
        for (int d = 1; d <= N; d++) {
      
            total_disk_used += total_used[d];
        }
        

        vector<int> selected_disks;  // ���ڴ洢ѡ��Ĵ���

        // ��һ������ǰ���������������Ҫ����
        for (int j = 0; j < 2; j++) {
            int main_disk = tag_main_disks[category][j];  // ��ȡ��ǰ��ǩ����Ҫ����
            if (total_used[main_disk] + size <= 0.95*V&& find(selected_disks.begin(), selected_disks.end(), main_disk) == selected_disks.end()) {  // ������̿ռ��㹻
                selected_disks.push_back(main_disk);  // �����̼���ѡ���б�
            }
            else {
                // ����ռ䲻�㣬�����ҵ�һ����ѡ����
                for (int d = 1; d <= N; d++) {
                    // ��鵱ǰ�����Ƿ�δ��ʹ�ò��ҿռ��㹻
                    if (total_used[d] + size <= 0.9*V &&
                        find(selected_disks.begin(), selected_disks.end(), d) == selected_disks.end()) {
                        selected_disks.push_back(d);  // ����ѡ���̼���ѡ���б�
                        break;
                    }
                }
            }
        }

        // �ڶ������ڸ�������ʹ����ѯ����Ϊ�����������������
        int& current_index1 = last_used_disk_index[category];  // ��ȡ��ǰ��ǩ�Ĵ�����ѯ����
        current_index1++;
        for (int attempts = 0; attempts < N; attempts++) {
            int current_index = (current_index1 % N) + 1;  // ��ѯ��һ������
            int candidate = current_index;
            // ����ô���δ��ʹ�ã����ҿռ��㹻����ѡ��ô���
            if (find(selected_disks.begin(), selected_disks.end(), candidate) == selected_disks.end() &&
                total_used[candidate] + size < 0.95*V&& (candidate!=7|| candidate != 9)) {
                selected_disks.push_back(candidate);  // ���ô��̼���ѡ���б�
                break;
            }
        }

        // ���˻��ƣ�����Ҳ������ʵĴ��̣�ǿ��ѡ��һ��δʹ�õĴ���
        if (selected_disks.size() < 3) {
            for (int d = N; d >=1; d--) {
                if (find(selected_disks.begin(), selected_disks.end(), d) == selected_disks.end() &&
                    total_used[d] + size <= 0.95*V) {
                    selected_disks.push_back(d);  // �����̼���ѡ���б�
                    if (selected_disks.size() == 3) break;
                }
            }
        }

        // ��������Ϊÿ������ִ��д�����
        for (int j = 1; j <= REP_NUM; j++) {
            int disk_id = selected_disks[j - 1];  // ��ȡ��ǰ�����Ĵ���ID
            object[id].replica[j] = disk_id;  // ���ø������ڴ���ID
            object[id].unit[j] = static_cast<int*>(malloc(sizeof(int) * (size + 1)));  // Ϊ��������洢�ռ�
            object[id].size = size;  // ���ö����С
            object[id].is_delete = false;  // ��Ƕ���δɾ��
            do_object_write(object[id].unit[j], disk[disk_id], size, id, j);  // ִ��д�����
        }

        printf("%d\n", id);  // �������ID
        for (int j = 1; j <= REP_NUM; j++) {
            printf("%d", object[id].replica[j]);  // ����������ڵĴ���ID
            for (int k = 1; k <= size; k++) {
                printf(" %d", object[id].unit[j][k]);  // ��������洢��λ����Ϣ
            }
            printf("\n");
        }
    }
    fflush(stdout);  // ˢ�������������ȷ�����������ʾ
}


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
vector<unordered_map<int, int>> disk_request_map(MAX_DISK_NUM); //����map

vector<int> current_request(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ����������
vector<int> current_object_id(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ����Ķ���
vector<int> current_replica(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ�����Ƕ���ļ��Ÿ���
vector<int> current_first_pos(MAX_DISK_NUM); //��¼ÿ��Ӳ�̵�ǰ���������׵�ַ

vector<int> current_haveread(MAX_DISK_NUM); //��¼ÿ��Ӳ���Ѿ��������˶��ٴζ�����
vector<int> current_disk_pos(MAX_DISK_NUM, 1); //��¼ÿ��Ӳ�̵�ǰ��ͷλ�ã���ʼ��ʱΪ1
vector<int> pAss(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫpass�Ĵ���
vector<int> jUmp(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫjump�Ĵ���
vector<int> rEad(MAX_DISK_NUM);               //��¼ÿ��Ӳ�̵�ǰ��Ҫread�Ĵ���



//��һ������������Ѿ������set��ɾ��
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
        current_request[i] = 0; // ��������״̬
        current_object_id[i] = 0;
        current_replica[i] = 0;
        current_first_pos[i] = 0;
        return;
    }


    current_request[i] = return_request;                       //���¸ô��̵�ǰ����������  
    current_object_id[i] = request[current_request[i]].object_id;//���¸ô��̵�ǰ����������
    for (int j = 1; j <= 3; j++) {
        if (object[current_object_id[i]].replica[j] == i) {
            current_replica[i] = j;                              //���¸ô��̵�ǰ��������񸱱���
            break;
        }
    }
    current_first_pos[i] = object[current_object_id[i]].unit[current_replica[i]][1];//���¸ô��̵�ǰ����������׵�ַ

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

int get_distance(int i) {
    int pos = (current_disk_pos[i] - 1) % V + 1; // ȷ��λ����[1, V]
    int target = current_first_pos[i];
    if (pos <= target) return target - pos;
    else return (V - pos) + target;
}

void read_action(int t)
{
    int n_read;
    int request_id, object_id;
    //ÿ��ʱ�䲽�������õ���Ŀ
    vector<int> current_token(MAX_DISK_NUM, 0);//��¼ÿ��Ӳ�̵�ǰʱ�䲽���ĵ�������
    int nums = 0;               //��¼��ǰʱ�䲽��ɵ��������
    queue<int> request_ans;   //��¼��ǰʱ�䲽��ɵ�����

    scanf("%d", &n_read);

    for (int i = 1; i <= n_read; i++) {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;
        request[request_id].prev_id = object[object_id].last_request_point;
        object[object_id].last_request_point = request_id;
        request[request_id].is_done = false;
        request[request_id].born_time = t; // ��¼���������ʱ��Ƭ
        //������������������������������map��
        for (int j = 1; j <= 3; j++) {
            if (object[object_id].is_continuous[j]) {                 //����ø������������������������ջ��
                int disk_index = object[object_id].replica[j];        //�ҵ��洢��ǰ����j�Ÿ�����Ӳ�̺�
                int first_pos = object[object_id].unit[j][1];         //�ҵ���ǰ�����׵�ַ
                disk_request_map[disk_index][first_pos] = request_id; //�ѵ�ǰ�����������ӦӲ�̵�map��
            }
        }
    }

    for (int i = 1; i <= N; i++) {
        while (1) {

            if (current_request[i] == 0) {             //����ǰӲ��δ��������
                get_mission(i,t);
                if (current_request[i] == 0) {
                    //��ǰӲ��û��������map��Ҳ�����������
                    printf("#\n");
                    break;
                }
                all_disk_map_erase(current_request[i]);
                if (get_distance(i) <= G) {
                    //����ǰ��ͷλ�þ������洢���׵�ַ<=G,����p+r�ķ�ʽ��
                    pAss[i] = get_distance(i);
                    rEad[i] = object[current_object_id[i]].size;
                }
                else {
                    //��j+r�ķ�ʽ��
                    jUmp[i] = 1;
                    rEad[i] = object[current_object_id[i]].size;
                }
            }
            else {//��ǰ�����ڴ�������
                //�ȴ���j��Ϊ
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
                //�ٴ���p��Ϊ
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
                //�ٴ���r��Ϊ
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
                    else { // �������
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
                        current_request[i] = 0; // ��������״̬
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

// ����̬������ڴ�
void clean() {
    for (int i = 0; i < MAX_OBJECT_NUM; i++) { // �������ж���
        for (int j = 1; j <= REP_NUM; j++) {
            if (object[i].unit[j] != nullptr) { // ����ڴ��ѷ���
                free(object[i].unit[j]); // �ͷ��ڴ�
                object[i].unit[j] = nullptr; // �ÿ�ָ��
            }
        }
    }
}

int main() {
    scanf("%d%d%d%d%d", &T, &M, &N, &V, &G);
    int slices = (T + FRE_PER_SLICING - 1) / FRE_PER_SLICING;

    // ��ȡԤ��������
    vector<vector<int>> fre_del(M + 1, vector<int>(slices + 1));
    vector<vector<int>> fre_write(M + 1, vector<int>(slices + 1));
    vector<vector<int>> fre_read(M + 1, vector<int>(slices + 1));

    // ��ȡɾ����
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slices; j++) {
            scanf("%d", &fre_del[i][j]);
        }
    }

    // ��ȡд����
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slices; j++) {
            scanf("%d", &fre_write[i][j]);
        }
    }

    // ��ȡ��ȡ����δʹ�õ��������룩
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= slices; j++) {
            scanf("%d", &fre_read[i][j]);
        }
    }

    // ����ÿ����ǩ�ġ�д-ɾ���ֵ��������
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

    // ��д-ɾ���ֵ��������
    sort(tag_max_diff_ranks.begin(), tag_max_diff_ranks.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first; // �����ֵ��������
        });

    // ���ִ��ǩ��С��ǩ��ǰ50%�ͺ�50%��
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
        // ==== �������ֶ������ǩ8��12�Ĵ��� ====
        if (tag == 8) {
            tag_main_disks[tag][0] = 7;
            tag_main_disks[tag][1] = 8;
            // ����ǩ8��ӵ�����7��8������ǩ�б�
            disk_main_tags[7].push_back(tag);
            disk_main_tags[8].push_back(tag);
            continue; // ������ѯ�߼�
        }
        if (tag == 12) {
            tag_main_disks[tag][0] = 9;
            tag_main_disks[tag][1] = 10;
            // ����ǩ12��ӵ�����9��10������ǩ�б�
            disk_main_tags[9].push_back(tag);
            disk_main_tags[10].push_back(tag);
            continue; // ������ѯ�߼�
        }
        // ==== ԭ����ѯ�߼�������7��8��9��10�Ŵ��̣�====
        for (int i = 0; i < 2; i++) {
            // �����������̣�7��8��9��10��
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

    // �����ض������ϱ�ǩ����ʼ����յ�
    for (int d = 1; d <= N; d++) {
        if (d == 1 || d == 3 || d == 5) {
            // ����1�š�3�š�5�Ŵ������������ǩ������
            vector<int> big_tags_on_disk;
            for (int tag : big_tags) {
                if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) != disk_main_tags[d].end()) {
                    big_tags_on_disk.push_back(tag);
                }
            }
            if (big_tags_on_disk.size() >= 2) {
                int tag1 = big_tags_on_disk[0]; // ��һ�����ǩ
                int tag2 = big_tags_on_disk[1]; // �ڶ������ǩ
                // ������ʼ����յ�
                swap(tag_region_start[d][tag1], tag_region_start[d][tag2]);
                swap(tag_region_end[d][tag1], tag_region_end[d][tag2]);
            }
        }
        else if (d == 7 || d == 9 || d == 1) {
            // ����7�Ŵ���������С��ǩ������
            vector<int> small_tags_on_disk;
            for (int tag : small_tags) {
                if (find(disk_main_tags[d].begin(), disk_main_tags[d].end(), tag) != disk_main_tags[d].end()) {
                    small_tags_on_disk.push_back(tag);
                }
            }
            if (small_tags_on_disk.size() >= 2) {
                int tag1 = small_tags_on_disk[0]; // ��һ��С��ǩ
                int tag2 = small_tags_on_disk[1]; // �ڶ���С��ǩ
                // ������ʼ����յ�
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