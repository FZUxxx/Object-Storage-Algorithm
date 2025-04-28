#include "prehanle.h"
#include <cstdio>

#define FRE_PER_SLICING (1800)

// �����ﶨ�� g_tag
Tag* tag = nullptr;
// Block �ṹ�嶨�壨���ڹ������

Tag::Tag() {}
Tag::~Tag() {}
// ���壨ʵ�ʷ����ڴ棩
std::vector <std::vector<TagInfo>> tags;

void Tag::process_data(const int& T, const int& M) {
    int value;
    int slice_count = (T - 1) / FRE_PER_SLICING + 1;

    // ��ʼ�����ݽṹ
    delete_ops.resize(M, std::vector<int>(slice_count));
    write_ops.resize(M, std::vector<int>(slice_count));
    read_ops.resize(M, std::vector<int>(slice_count));
    hot_ops.resize(M, std::vector<bool>(slice_count));
    delete_sums.resize(M, 0);
    write_sums.resize(M, 0);
    write_minus_delete.resize(M, 0);

    // --- �޸Ĳ��֣���ʼ��������Ա���� ---
    tag_head_pos.resize(M + 1, 0);
    tag_free_blocks.resize(M + 1);
    tagcount.resize(M + 1, 0);

    // ��ȡ delete_ops �������ܺ�
    for (int i = 0; i < M; i++) {
        int sum = 0;
        for (int j = 0; j < slice_count; j++) {
            scanf("%d", &value);
            delete_ops[i][j] = value;
            sum += value;
        }
        delete_sums[i] = sum;
    }

    // ��ȡ write_ops �������ܺ�
    for (int i = 0; i < M; i++) {
        int sum = 0;
        for (int j = 0; j < slice_count; j++) {
            scanf("%d", &value);
            write_ops[i][j] = value;
            sum += value;
        }
        write_sums[i] = sum;
    }

    // ��ȡ read_ops
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < slice_count; j++) {
            scanf("%d", &value);
            read_ops[i][j] = value;
        }
    }

    // ���� hot ״̬ �� д��-ɾ����ֵ
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < slice_count; j++) {
            hot_ops[i][j] = read_ops[i][j] >= write_ops[i][j];
        }
        write_minus_delete[i] = write_sums[i] - delete_sums[i];
    }
}

void Tag::compute_tag_regions(const int& N, const int& V, const int& M) {
    
    
    tag_region1.resize(M + 1);
    tag_region2.resize(M + 1);
    tag_regionstart.resize(M + 1);

    int total_region = 0;

    // ���� tag_region1[i]
    for (int i = 1; i <= M; i++) {
        tag_region1[i] = write_minus_delete[i - 1] * 3 / N;
        total_region += tag_region1[i];
    }

    // ����ƽ������
    region2 = (V - M - total_region) / M;

    // ���� tag_region2[i]
    for (int i = 1; i <= M; i++) {
        tag_region2[i] = tag_region1[i] + region2;
    }

    // ����ÿ����ǩ����ʼ�����Ų���ʼ����ͷ
    int current_start = 1;
    for (int i = 1; i <= M; i++) {
        tag_regionstart[i] = current_start;
        tag_head_pos[i] = current_start; // ��ʼ����ͷλ��Ϊ������ʼ
        current_start += tag_region2[i];
    }
    // ��ʼ����ͷλ��Ϊÿ����ǩ�������ʼλ��
    for (int disk_id = 1; disk_id <= N; disk_id++) {
        for (int tag_id = 1; tag_id <= M; tag_id++) {
            tag_point[disk_id][tag_id] = tag_regionstart[tag_id];
        }
    }
}


