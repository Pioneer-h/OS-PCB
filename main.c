#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PROC 100                 // 最大进程数量限制

/*
 * PCB 进程控制块（Process Control Block）
 * 操作系统中每个进程都有这样一个数据结构，记录进程的所有信息
 */
struct PCB {
    char name[10];                   // 进程名称（如 P1, P2）
    int arriveTime;                  // 到达时间 - 进程什么时候进入系统
    int burstTime;                   // 运行时间 - 需要在CPU上运行多久才能完成
    int remainTime;                  // 剩余时间 - 还剩多久没运行完
    int priority;                    // 优先权 - 数值越小优先级越高
    int startTime;                  // 开始时间 - 第一次被CPU调度的时间点
    int finishTime;                 // 完成时间 - 进程运行结束的时间点
    int turnaroundTime;             // 周转时间 = 完成时间 - 到达时间，表示进程在系统中停留了多久
    float weightedTaTime;           // 带权周转时间 = 周转时间 / 运行时间，反映调度质量
    struct PCB *next;               // 链表指针 - 指向下一个进程，用于就绪队列
};

struct PCB backupArray[MAX_PROC];  // 备份数组 - 保存所有进程原始数据，每次调度都从这里复制
int backupCount = 0;               // 当前保存的进程总数
int activeFlag[MAX_PROC];         // 标记进程是否已经入队（0=未入队，1=已入队）
int currentTime = 0;               // 当前系统时间，模拟调度过程中的时钟推进

/*
 * 初始化系统 - 将所有备份数组清零
 * 程序启动时调用一次，确保所有数据从干净状态开始
 */
void initSystem(void) {
    int i;
    for (i = 0; i < MAX_PROC; i++) {
        strcpy(backupArray[i].name, "");
        backupArray[i].arriveTime = 0;
        backupArray[i].burstTime = 0;
        backupArray[i].remainTime = 0;
        backupArray[i].priority = 0;
        backupArray[i].startTime = -1;        // -1 表示尚未开始运行
        backupArray[i].finishTime = -1;       // -1 表示尚未完成
        backupArray[i].turnaroundTime = 0;
        backupArray[i].weightedTaTime = 0.0f;
        backupArray[i].next = NULL;
        activeFlag[i] = 0;                    // 0 表示还没有入队
    }
    backupCount = 0;
    currentTime = 0;
}

/*
 * 按到达时间升序排序 - 冒泡排序
 * 调度前必须排序，确保进程按到达顺序处理
 */
void sortBackup(void) {
    int i, j;
    // 冒泡排序：双重循环比较相邻元素
    for (i = 0; i < backupCount; i++) {
        for (j = i+1; j < backupCount; j++) {
            // 如果后面的进程到达时间更早，交换位置
            if (backupArray[j].arriveTime < backupArray[i].arriveTime) {
                struct PCB temp = backupArray[i];   // 用临时变量保存
                backupArray[i] = backupArray[j];
                backupArray[j] = temp;
            }
        }
    }
}
/*
 * 从文件加载进程数据
 * 格式：第一行是进程数量，之后每行：名称 到达时间 运行时间 优先权
 * 返回 1 表示加载成功，0 表示失败
 */
int load_data(void) {
    FILE *fp = fopen("processes.txt", "r");   // 以只读方式打开文件
    if (fp == NULL) return 0;                 // 文件不存在则返回失败（0）
    // 读取第一行：进程总数
    if (fscanf(fp, "%d", &backupCount) != 1) { fclose(fp); return 0; }
    // 检查数量是否合法（0~MAX_PROC之间）
    if (backupCount <= 0 || backupCount > MAX_PROC) { fclose(fp); return 0; }
    int i;
    for (i = 0; i < backupCount; i++) {
        char id[10]; int arr, bst, prio;
        // 每行读取：名称 到达时间 运行时间 优先权
        if (fscanf(fp, "%s %d %d %d", id, &arr, &bst, &prio) != 4) break;
        // 把读取到的数据填入备份数组
        strcpy(backupArray[i].name, id);
        backupArray[i].arriveTime = arr;
        backupArray[i].burstTime = bst;
        backupArray[i].priority = prio;
        backupArray[i].remainTime = bst;       // 初始剩余时间 = 运行时间
        backupArray[i].startTime = -1;         // -1 表示还没有开始运行
        backupArray[i].finishTime = -1;        // -1 表示还没有完成
        backupArray[i].turnaroundTime = 0;     // 初始周转时间 = 0
        backupArray[i].weightedTaTime = 0.0f;  // 初始带权周转时间 = 0
        backupArray[i].next = NULL;            // 链表指针初始为空
    }
    fclose(fp);                                // 关闭文件
    sortBackup();                              // 加载后按到达时间排序
    return 1;                                  // 返回1表示加载成功
}

/*
 * 保存进程数据到文件
 * 每次修改进程数据后自动保存，确保数据持久化
 */
void save_data(void) {
    FILE *fp = fopen("processes.txt", "w");   // 以写方式打开（会覆盖原有内容）
    if (fp == NULL) { printf("保存失败！\n"); return; }
    fprintf(fp, "%d\n", backupCount);         // 第一行写入进程数量
    int i;
    // 逐行写入每个进程的4个属性
    for (i = 0; i < backupCount; i++)
        fprintf(fp, "%s %d %d %d\n", 
                backupArray[i].name,          // 进程名称
                backupArray[i].arriveTime,    // 到达时间
                backupArray[i].burstTime,     // 运行时间
                backupArray[i].priority);     // 优先权
    fclose(fp);                               // 关闭文件
    printf("已保存到 processes.txt\n");
}

/*
 * 初始化默认进程 - 5 个示例进程，方便快速测试
 * 当 processes.txt 不存在时自动使用
 */
void init_default_processes(void) {
    // 逐行初始化5个示例进程，方便快速测试
    // 格式：名称 到达时间 运行时间 优先权
    strcpy(backupArray[0].name, "P1"); backupArray[0].arriveTime=0; backupArray[0].burstTime=5; backupArray[0].priority=3;
    strcpy(backupArray[1].name, "P2"); backupArray[1].arriveTime=2; backupArray[1].burstTime=2; backupArray[1].priority=5;
    strcpy(backupArray[2].name, "P3"); backupArray[2].arriveTime=4; backupArray[2].burstTime=3; backupArray[2].priority=1;
    strcpy(backupArray[3].name, "P4"); backupArray[3].arriveTime=5; backupArray[3].burstTime=4; backupArray[3].priority=4;
    strcpy(backupArray[4].name, "P5"); backupArray[4].arriveTime=6; backupArray[4].burstTime=1; backupArray[4].priority=2;
    backupCount = 5;          // 设置进程总数为5
    sortBackup();             // 按到达时间排序
    save_data();              // 保存到文件，下次启动可以直接加载
}

/*
 * 用户手动输入所有进程
 * 用户依次输入每个进程的名称、到达时间、运行时间、优先权
 */
void inputProcesses(void) {
    int i;
    printf("\n请输入进程数量: ");
    scanf("%d", &backupCount);              // 用户输入进程总数
    if (backupCount <= 0 || backupCount > MAX_PROC) { printf("无效的进程数量！\n"); backupCount=0; return; }
    for (i = 0; i < backupCount; i++) {
        // 输入每个进程的4个属性
        printf("进程 %d: 名称 到达时间 运行时间 优先权 ", i+1);
        scanf("%s %d %d %d", backupArray[i].name, &backupArray[i].arriveTime, &backupArray[i].burstTime, &backupArray[i].priority);
        backupArray[i].remainTime = backupArray[i].burstTime;  // 初始剩余时间 = 总运行时间
        backupArray[i].startTime = -1;       // 还未开始
        backupArray[i].finishTime = -1;      // 还未完成
        backupArray[i].turnaroundTime = 0;
        backupArray[i].weightedTaTime = 0.0f;
        backupArray[i].next = NULL;          // 链表指针初始为空
    }
    sortBackup();     // 按到达时间排序，方便后续调度
    save_data();      // 保存到文件
    printf("完成！\n");
}

/*
 * 添加单个进程 - 一次只添加一个，方便逐步增加
 */
void add_process(void) {
    // 检查进程数量是否已满
    if (backupCount >= MAX_PROC) { printf("进程已满\n"); return; }
    // 提示用户输入进程信息
    printf("输入进程名称: "); scanf("%s", backupArray[backupCount].name);
    printf("输入到达时间: "); scanf("%d", &backupArray[backupCount].arriveTime);
    printf("输入运行时间: "); scanf("%d", &backupArray[backupCount].burstTime);
    printf("输入优先权: "); scanf("%d", &backupArray[backupCount].priority);
    // 初始化所有运行时状态
    backupArray[backupCount].remainTime = backupArray[backupCount].burstTime;
    backupArray[backupCount].startTime = -1;
    backupArray[backupCount].finishTime = -1;
    backupArray[backupCount].turnaroundTime = 0;
    backupArray[backupCount].weightedTaTime = 0.0f;
    backupArray[backupCount].next = NULL;
    backupCount++;             // 进程总数加1
    sortBackup();              // 重新按到达时间排序
    save_data();               // 保存到文件
    printf("已添加。\n");
}
/*
 * 显示当前所有进程列表
 * 显示进程的基本信息：名称、到达时间、运行时间、优先权
 */
void show_process_list(void) {
    int i;
    if (backupCount <= 0) { printf("没有进程\n"); return; }  // 没有进程就不显示
    printf("\n===== 进程列表 =====\n");
    // 打印表头
    printf("%-10s %-10s %-10s %-10s\n", "名称","到达","运行","优先权");
    // 逐行打印每个进程的信息
    for (i = 0; i < backupCount; i++)
        printf("%-10s %-10d %-10d %-10d\n", 
               backupArray[i].name,          // 进程名称
               backupArray[i].arriveTime,    // 到达时间
               backupArray[i].burstTime,     // 运行时间
               backupArray[i].priority);     // 优先权
    printf("\n");
}
/*
 * 随机生成进程 - 全部随机生成，方便大量测试
 * 到达时间：0~10，运行时间：1~8，优先权：1~5
 */
void random_generate(void) {
    int n, i;
    printf("生成多少个进程？ ");
    scanf("%d", &n);
    if (n <= 0 || n > MAX_PROC) { printf("无效数量！\n"); return; }
    srand(time(NULL));          // 初始化随机种子，确保每次不同的随机结果
    backupCount = n;
    for (i = 0; i < n; i++) {
        sprintf(backupArray[i].name, "P%d", i+1);          // 名称为 P1, P2, P3...
        backupArray[i].arriveTime = rand() % 11;           // 到达时间 0~10
        backupArray[i].burstTime = rand() % 8 + 1;         // 运行时间 1~8
        backupArray[i].priority = rand() % 5 + 1;          // 优先权 1~5
        backupArray[i].remainTime = backupArray[i].burstTime;
        backupArray[i].startTime = -1;
        backupArray[i].finishTime = -1;
        backupArray[i].turnaroundTime = 0;
        backupArray[i].weightedTaTime = 0.0f;
        backupArray[i].next = NULL;
    }
    sortBackup();     // 按到达时间排序
    save_data();      // 保存到文件
    printf("随机生成完成！\n");
    show_process_list();   // 显示生成的进程列表
}

/*
 * 重置进程运行状态 - 每次调度算法运行前调用
 * 把所有运行时的数据（开始时间、完成时间等）恢复初始状态
 */
void resetProcesses(void) {
    int i;
    for (i = 0; i < backupCount; i++) {
        backupArray[i].startTime = -1;          // 重置为"未开始"
        backupArray[i].finishTime = -1;         // 重置为"未完成"
        backupArray[i].turnaroundTime = 0;
        backupArray[i].weightedTaTime = 0.0f;
        backupArray[i].remainTime = backupArray[i].burstTime;  // 恢复剩余时间
        backupArray[i].next = NULL;            // 清除链表指针
    }
}

/*
 * 重置入队标记 - 所有进程标记为"未入队"
 * 配合调度算法使用，确保每个进程只入队一次
 */
void resetActiveFlags(void) {
    int i;
    for (i = 0; i < MAX_PROC; i++) activeFlag[i] = 0;
}

/*
 * 链表尾插法 - 把节点加到队列末尾（FIFO先入先出）
 * qh: 队列头指针, qt: 队列尾指针, node: 要插入的节点
 * 用于 FCFS、RR、MFQ 的就绪队列
 */
void pushBack(struct PCB **qh, struct PCB **qt, struct PCB *node) {
    node->next = NULL;               // 新节点作为最后一个，没有后继
    if (*qt == NULL) {               // 如果队列为空，头尾都指向新节点
        *qh = node; *qt = node;
    } else {                         // 队列非空，接在尾部
        (*qt)->next = node;
        *qt = node;                  // 更新尾指针
    }
}

/*
 * 按优先权插入 - 数值越大优先级越低
 * 用于静态优先权调度的就绪队列，高优先权（数值小）的排前面
 */
void pushPriority(struct PCB **qh, struct PCB **qt, struct PCB *node) {
    struct PCB *cur = *qh, *prev = NULL;
    // 找到第一个优先权比新节点低的（数值大），插入它前面
    while (cur != NULL && cur->priority > node->priority) {
        prev = cur;
        cur = cur->next;
    }
    node->next = cur;                // 插入到 cur 前面
    if (prev == NULL) *qh = node;   // 插入到队头
    else prev->next = node;          // 插入到中间
    if (cur == NULL) *qt = node;    // 插入到队尾，更新尾指针
}

/*
 * 链表头出队 - 取出队列第一个节点
 * 返回取出节点的指针，同时更新队列头
 */
struct PCB *popFront(struct PCB **qh, struct PCB **qt) {
    struct PCB *node = *qh;          // 取出队头节点
    if (node == NULL) return NULL;   // 队列为空
    *qh = node->next;                // 头指针移到下一个
    if (*qh == NULL) *qt = NULL;    // 如果队列空了，尾指针也置空
    node->next = NULL;               // 断开节点和队列的连接
    return node;
}

/*
 * 检查当前时刻已到达但未入队的进程，加入FIFO就绪队列
 * 每次时钟推进后都要调用，把新到达的进程入队
 */
void checkAndEnqueue(struct PCB **qh, struct PCB **qt) {
    int i;
    for (i = 0; i < backupCount; i++) {
        // 如果进程还没入队，且到达时间 <= 当前时间，说明已经到达了
        if (activeFlag[i] == 0 && backupArray[i].arriveTime <= currentTime) {
            struct PCB *node = (struct PCB *)malloc(sizeof(struct PCB));
            // 从备份数组复制一份到链表节点
            strcpy(node->name, backupArray[i].name);
            node->arriveTime = backupArray[i].arriveTime;
            node->burstTime = backupArray[i].burstTime;
            node->remainTime = backupArray[i].remainTime;
            node->priority = backupArray[i].priority;
            node->startTime = backupArray[i].startTime;
            node->finishTime = backupArray[i].finishTime;
            node->turnaroundTime = backupArray[i].turnaroundTime;
            node->weightedTaTime = backupArray[i].weightedTaTime;
            node->next = NULL;
            pushBack(qh, qt, node);       // 加入队尾
            activeFlag[i] = 1;            // 标记为已入队，避免重复入队
        }
    }
}

/*
 * 检查当前时刻已到达但未入队的进程，按优先级加入就绪队列
 * 用于静态优先权调度，每次时钟推进后调用
 */
void checkAndEnqueuePriority(struct PCB **qh, struct PCB **qt) {
    int i;
    for (i = 0; i < backupCount; i++) {
        // 检查：未入队 且 已到达
        if (activeFlag[i] == 0 && backupArray[i].arriveTime <= currentTime) {
            struct PCB *node = (struct PCB *)malloc(sizeof(struct PCB));  // 分配新节点
            if (node == NULL) { printf("内存分配失败！\n"); exit(1); }
            // 从备份数组复制数据到链表节点
            strcpy(node->name, backupArray[i].name);
            node->arriveTime = backupArray[i].arriveTime;
            node->burstTime = backupArray[i].burstTime;
            node->remainTime = backupArray[i].remainTime;
            node->priority = backupArray[i].priority;
            node->startTime = backupArray[i].startTime;
            node->finishTime = backupArray[i].finishTime;
            node->turnaroundTime = backupArray[i].turnaroundTime;
            node->weightedTaTime = backupArray[i].weightedTaTime;
            node->next = NULL;
            pushPriority(qh, qt, node);   // 按优先级插入
            activeFlag[i] = 1;            // 标记为已入队
        }
    }
}

/*
 * 找到所有未到达进程中最早的到达时间
 * 当就绪队列为空时，系统会直接跳到这个时间，跳过CPU空闲时间
 */
int findNextArrivalTime(void) {
    int i, nextTime = -1;              // -1 表示还没有找到
    for (i = 0; i < backupCount; i++) {
        if (activeFlag[i] == 0) {      // 只检查还没入队的进程
            // 找最小的到达时间
            if (nextTime < 0 || backupArray[i].arriveTime < nextTime)
                nextTime = backupArray[i].arriveTime;
        }
    }
    return nextTime;                   // 返回最早到达时间，-1表示所有进程都已入队
}

/*
 * 将链表节点中的调度结果复制回备份数组
 * 因为我们用链表节点做调度，结果要写回备份才能打印输出
 */
void copyResultToBackup(struct PCB *node) {
    int i;
    // 遍历备份数组，找到对应名称的进程
    for (i = 0; i < backupCount; i++) {
        if (strcmp(backupArray[i].name, node->name) == 0) {
            // 把调度结果写回备份数组，方便后续打印
            backupArray[i].startTime = node->startTime;
            backupArray[i].finishTime = node->finishTime;
            backupArray[i].turnaroundTime = node->turnaroundTime;
            backupArray[i].weightedTaTime = node->weightedTaTime;
            backupArray[i].remainTime = 0;     // 已完成，剩余时间为0
            break;                              // 找到了就退出循环
        }
    }
}

/*
 * 释放链表所有节点内存 - 避免内存泄漏
 * 调度完成后调用，释放链表节点
 */
void freeList(struct PCB *head) {
    struct PCB *cur = head;
    while (cur != NULL) {
        struct PCB *next = cur->next;
        free(cur);      // 释放每个节点
        cur = next;
    }
}

/*
 * 打印调度结果表格 - 格式化输出所有进程信息和统计结果
 */
void printResultTable(const char *name) {
    int i;
    float sumT = 0.0f, sumW = 0.0f;       // sumT: 总周转时间, sumW: 总带权周转时间
    printf("\n===== %s 调度结果 =====\n", name);
    // 打印表头，%-10s表示左对齐宽度10
    printf("%-10s %-8s %-6s %-8s %-8s %-10s %-8s %-8s\n",
           "进程","到达","运行","开始","完成","周转","带权周转","优先权");
    // 逐行打印每个进程的调度结果
    for (i = 0; i < backupCount; i++) {
        printf("%-10s %-8d %-6d %-8d %-8d %-10d %-8.2f %-8d\n",
               backupArray[i].name,         // 进程名称
               backupArray[i].arriveTime,   // 到达时间
               backupArray[i].burstTime,    // 运行时间
               backupArray[i].startTime,    // 开始时间
               backupArray[i].finishTime,   // 完成时间
               backupArray[i].turnaroundTime,   // 周转时间
               backupArray[i].weightedTaTime,   // 带权周转时间（保留两位小数）
               backupArray[i].priority);        // 优先权
        sumT += backupArray[i].turnaroundTime;  // 累加周转时间
        sumW += backupArray[i].weightedTaTime;  // 累加带权周转时间
    }
    // 打印统计结果（平均值）
    if (backupCount > 0) {
        printf("\n平均周转时间: %.2f\n", sumT / backupCount);
        printf("平均带权周转时间: %.2f\n", sumW / backupCount);
    }
}

/*
 * FCFS 先来先服务调度算法
 * 原理：哪个进程先到达，哪个先运行，直到完成
 * 特点：非抢占式，实现简单，对短进程不友好
 */
void fcfs(void) {
    struct PCB *qh = NULL, *qt = NULL;  // 就绪队列头指针、尾指针
    int done = 0;                        // 已完成进程数
    resetProcesses(); resetActiveFlags(); currentTime = 0;  // 初始化
    while (done < backupCount) {         // 直到所有进程完成
        checkAndEnqueue(&qh, &qt);       // 将当前已到达的进程全部入队
        if (qh == NULL) {               // 就绪队列为空，说明还没有进程到达
            int nt = findNextArrivalTime();
            if (nt < 0) break;         // 所有进程都完成了
            currentTime = nt;           // 直接跳到下一个进程到达时间
            continue;
        }
        struct PCB *node = popFront(&qh, &qt);  // 取出队头第一个进程
        if (node->startTime < 0) node->startTime = currentTime;  // 记录开始时间
        currentTime += node->remainTime;  // 进程一直运行直到完成，推进时钟
        node->finishTime = currentTime;   // 记录完成时间
        node->remainTime = 0;             // 剩余时间清0
        // 计算周转时间和带权周转时间
        node->turnaroundTime = node->finishTime - node->arriveTime;
        node->weightedTaTime = (node->burstTime > 0) ? (float)node->turnaroundTime / node->burstTime : 0.0f;
        copyResultToBackup(node);  // 结果写回备份数组
        done++;  // 已完成计数+1
        free(node);  // 释放链表节点
    }
    freeList(qh);  // 释放队列中剩余节点
    printResultTable("FCFS");  // 打印结果
}

/*
 * 静态优先权调度算法
 * 原理：每个进程优先权固定，每次选优先权最高的运行，直到完成
 * 特点：非抢占式，优先权高的先运行，优先权是静态确定的
 */
void priority(void) {
    struct PCB *qh = NULL, *qt = NULL;  // 就绪队列（按优先权排序）
    int done = 0;
    resetProcesses(); resetActiveFlags(); currentTime = 0;
    while (done < backupCount) {
        checkAndEnqueuePriority(&qh, &qt);  // 按优先级插入队列
        if (qh == NULL) {
            int nt = findNextArrivalTime();
            if (nt < 0) break;
            currentTime = nt;
            continue;
        }
        struct PCB *node = popFront(&qh, &qt);  // 取出队头（就是优先权最高的）
        if (node->startTime < 0) node->startTime = currentTime;
        currentTime += node->remainTime;  // 一直运行到完成
        node->finishTime = currentTime;
        node->remainTime = 0;
        node->turnaroundTime = node->finishTime - node->arriveTime;
        node->weightedTaTime = (node->burstTime > 0) ? (float)node->turnaroundTime / node->burstTime : 0.0f;
        copyResultToBackup(node);
        done++;
        free(node);
    }
    freeList(qh);
    printResultTable("Static Priority");
}

/*
 * RR 时间片轮转调度算法
 * 原理：每个进程轮流运行一个时间片，时间片用完就回到队尾
 * 特点：抢占式，每个进程公平分配CPU时间，响应快
 */
void rr(void) {
    struct PCB *qh = NULL, *qt = NULL;
    int done = 0, ts;
    printf("请输入 RR 时间片大小: ");
    scanf("%d", &ts);
    if (ts <= 0) ts = 1;                // 时间片至少为1
    resetProcesses(); resetActiveFlags(); currentTime = 0;
    while (done < backupCount) {
        checkAndEnqueue(&qh, &qt);       // 先检查新到达的进程
        if (qh == NULL) {
            int nt = findNextArrivalTime();
            if (nt < 0) break;
            currentTime = nt;
            continue;
        }
        struct PCB *node = popFront(&qh, &qt);  // 取队头
        if (node->startTime < 0) node->startTime = currentTime;
        if (node->remainTime > ts) {     // 剩余时间 > 时间片，不能一次运行完
            currentTime += ts;           // 运行一个时间片
            node->remainTime -= ts;      // 减少剩余时间
            checkAndEnqueue(&qh, &qt);   // 运行期间可能有新进程到达
            pushBack(&qh, &qt, node);    // 未完成，放回队尾等待下次
        } else {                         // 剩余时间 <= 时间片，可以一次运行完
            currentTime += node->remainTime;
            node->finishTime = currentTime;
            node->remainTime = 0;
            node->turnaroundTime = node->finishTime - node->arriveTime;
            node->weightedTaTime = (node->burstTime > 0) ? (float)node->turnaroundTime / node->burstTime : 0.0f;
            copyResultToBackup(node);
            done++;
            free(node);
        }
    }
    freeList(qh);
    printResultTable("RR");
}

/*
 * MFQ 多级反馈队列调度算法
 * 原理：有3个级别的队列，每个队列时间片不同（1, 2, 4）
 *       新进程先进入最高级队列，用完时间片就降级
 *       高级队列优先调度，但每个队列时间片更短
 * 特点：综合了短作业优先和时间片轮转的优点
 */
void mfq(void) {
    struct PCB *qh[3] = {NULL,NULL,NULL};  // 三级队列，每级一个头指针
    struct PCB *qt[3] = {NULL,NULL,NULL};  // 三级队列，每级一个尾指针
    int ts[3] = {1,2,4};                  // 三级队列的时间片：1, 2, 4
    int done = 0, lv;                     // done: 完成数, lv: 当前调度的队列级别
    resetProcesses(); resetActiveFlags(); currentTime = 0;
    while (done < backupCount) {
        checkAndEnqueue(&qh[0], &qt[0]);  // 新进程先进入最高级队列（级别0）
        // 从高到低查找有进程的队列
        lv = -1;
        if (qh[0] != NULL) lv = 0;
        else if (qh[1] != NULL) lv = 1;
        else if (qh[2] != NULL) lv = 2;
        if (lv < 0) {                      // 所有队列都为空
            int nt = findNextArrivalTime();
            if (nt < 0) break;
            currentTime = nt;
            continue;
        }
        struct PCB *node = popFront(&qh[lv], &qt[lv]);  // 从当前级别队列取队头
        if (node->startTime < 0) node->startTime = currentTime;
        if (node->remainTime > ts[lv]) {   // 当前时间片不够完成
            currentTime += ts[lv];          // 运行一个时间片
            node->remainTime -= ts[lv];
            checkAndEnqueue(&qh[0], &qt[0]);  // 期间可能有新进程到达
            if (lv < 2)                    // 没有到最低级，降到下一级
                pushBack(&qh[lv+1], &qt[lv+1], node);
            else                           // 已经是最低级，保持在最低级
                pushBack(&qh[2], &qt[2], node);
        } else {                           // 当前时间片足够完成
            currentTime += node->remainTime;
            node->finishTime = currentTime;
            node->remainTime = 0;
            node->turnaroundTime = node->finishTime - node->arriveTime;
            node->weightedTaTime = (node->burstTime > 0) ? (float)node->turnaroundTime / node->burstTime : 0.0f;
            copyResultToBackup(node);
            done++;
            free(node);
        }
    }
    freeList(qh[0]); freeList(qh[1]); freeList(qh[2]);  // 释放三级队列
    printResultTable("MFQ");
}

/*
 * 主函数 - 程序入口
 * 显示菜单，根据用户选择调用不同的调度算法
 */
int main(void) {
    int choice;
    system("chcp 65001 >nul");          // 设置控制台为UTF-8编码，解决中文乱码
    initSystem();                        // 初始化系统
    if (!load_data()) {                  // 尝试从文件加载进程数据
        printf("无法加载 processes.txt，使用默认的 5 个进程\n");
        init_default_processes();        // 加载失败，使用默认进程
    }
    while (1) {                          // 主循环，显示菜单直到用户选择退出
        printf("\n===== 进程调度模拟系统 =====\n");
        printf("1. 显示进程列表\n");
        printf("2. FCFS\n");
        printf("3. Static Priority\n");
        printf("4. RR\n");
        printf("5. MFQ\n");
        printf("6. 添加进程\n");
        printf("7. 随机生成进程\n");
        printf("8. 重新输入所有进程\n");
        printf("9. 保存并退出\n");
        printf("请选择: ");
        if (scanf("%d", &choice) != 1) { // 输入不是数字，防止死循环
            printf("输入无效！\n");
            while (getchar() != '\n');   // 清空输入缓冲区
            continue;
        }
        if (choice == 1) show_process_list();
        else if (choice == 2) { if (backupCount<=0) printf("请先输入进程！\n"); else fcfs(); }
        else if (choice == 3) { if (backupCount<=0) printf("请先输入进程！\n"); else priority(); }
        else if (choice == 4) { if (backupCount<=0) printf("请先输入进程！\n"); else rr(); }
        else if (choice == 5) { if (backupCount<=0) printf("请先输入进程！\n"); else mfq(); }
        else if (choice == 6) add_process();
        else if (choice == 7) random_generate();
        else if (choice == 8) inputProcesses();
        else if (choice == 9) { save_data(); break; }  // 保存数据并退出循环
        else printf("选择无效！\n");
    }
    return 0;
}