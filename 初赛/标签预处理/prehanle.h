#pragma once
#include <vector>
#include <set>
#include <array>
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
// Block 结构体定义（用于管理空闲块）
struct Block {
    int start;
    int end;
    int size() const { return end - start + 1; }
    bool operator<(const Block& other) const {
        return start < other.start; // 按起始位置排序
    }
};
struct TagInfo {
    std::set<Block> free_blocks;      // 标签专属的连续空闲块集合
    std::array<std::set<Block>, 7> size_queues; // 索引1-6对应不同块大小
};
extern std::vector <std::vector<TagInfo>> tags; // 每个元素对应一个标签
extern std::vector<std::vector<int>>  tag_point;
class Tag {
public:
    std::vector<std::vector<int>> read_ops;
    std::vector<std::vector<int>> write_ops;
    std::vector<std::vector<int>> delete_ops;
    std::vector<std::vector<bool>> hot_ops;

    std::vector<int> delete_sums;           // 每个标签删除操作的总和
    std::vector<int> write_sums;            // 每个标签写入操作的总和
    std::vector<int> write_minus_delete;    // 写入 - 删除 差值

    std::vector<int> tag_region1;           // 每个标签占用的区域（初步）
    std::vector<int> tag_region2;           // 每个标签占用的区域（最终分配后）
    int region2;
    std::vector<int> tag_regionstart;       // 每个标签的起始区域编号（从1开始）

    // --- 修改部分：新增成员变量 ---
    std::vector<int> tag_head_pos;          // 每个标签的写入磁头位置
    std::vector<std::set<Block>> tag_free_blocks; // 每个标签的空闲块集合
    std::vector<int> tagcount;              // 每个标签的全局计数器
    


    Tag();
    ~Tag();

    void process_data(const int& T, const int& M);
    void compute_tag_regions(const int& N, const int& V, const int& M);
};

// 声明一个全局的 Tag 对象，但这里不分配内存
extern Tag* tag;