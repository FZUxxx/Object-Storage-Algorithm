#pragma once
#include <vector>
#include <set>
#include <array>
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
// Block �ṹ�嶨�壨���ڹ�����п飩
struct Block {
    int start;
    int end;
    int size() const { return end - start + 1; }
    bool operator<(const Block& other) const {
        return start < other.start; // ����ʼλ������
    }
};
struct TagInfo {
    std::set<Block> free_blocks;      // ��ǩר�����������п鼯��
    std::array<std::set<Block>, 7> size_queues; // ����1-6��Ӧ��ͬ���С
};
extern std::vector <std::vector<TagInfo>> tags; // ÿ��Ԫ�ض�Ӧһ����ǩ
extern std::vector<std::vector<int>>  tag_point;
class Tag {
public:
    std::vector<std::vector<int>> read_ops;
    std::vector<std::vector<int>> write_ops;
    std::vector<std::vector<int>> delete_ops;
    std::vector<std::vector<bool>> hot_ops;

    std::vector<int> delete_sums;           // ÿ����ǩɾ���������ܺ�
    std::vector<int> write_sums;            // ÿ����ǩд��������ܺ�
    std::vector<int> write_minus_delete;    // д�� - ɾ�� ��ֵ

    std::vector<int> tag_region1;           // ÿ����ǩռ�õ����򣨳�����
    std::vector<int> tag_region2;           // ÿ����ǩռ�õ��������շ����
    int region2;
    std::vector<int> tag_regionstart;       // ÿ����ǩ����ʼ�����ţ���1��ʼ��

    // --- �޸Ĳ��֣�������Ա���� ---
    std::vector<int> tag_head_pos;          // ÿ����ǩ��д���ͷλ��
    std::vector<std::set<Block>> tag_free_blocks; // ÿ����ǩ�Ŀ��п鼯��
    std::vector<int> tagcount;              // ÿ����ǩ��ȫ�ּ�����
    


    Tag();
    ~Tag();

    void process_data(const int& T, const int& M);
    void compute_tag_regions(const int& N, const int& V, const int& M);
};

// ����һ��ȫ�ֵ� Tag ���󣬵����ﲻ�����ڴ�
extern Tag* tag;