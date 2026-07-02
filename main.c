#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define N 10
#define MAX_ORDER 200

/*
 * PCB 结构体（Process Control Block，进程控制块）
 * 
 * 这是整个程序的核心数据结构，每个进程都有这样一个"信息卡片"，
 * 记录了进程的所有信息，操作系统就是靠这些信息来调度进程的。
 * 
 * 字段说明：
 *   id       - 进程名称，如 "P1", "P2"（字符串）
 *   arrive   - 到达时间：进程什么时候进入系统（越小越早到）
 *   burst    - 运行时间：进程需要运行多久才能完成（也叫"服务时间"）
 *   priority - 优先级：数字越小优先级越高（用于 MLFQ 调度）
 *   start    - 开始运行时间：进程第一次被 CPU 执行的时间点（初始 -1 表示未开始）
 *   finish   - 完成时间：进程运行结束的时间点
 *   remain   - 剩余运行时间：进程还剩多少时间没跑完（初始 = burst）
 *   wait     - 等待时间：进程在就绪队列中等待的总时间
 *   level    - 队列级别：用于 MLFQ 多级反馈队列（0最高优先级，2最低）
 */
struct pcb
{
    char id[10];
    int arrive;
    int burst;
    int priority;
    int start;
    int finish;
    int remain;
    int wait;
    int level;
};

void save_data(struct pcb p[], int count);
int load_data(struct pcb p[]);

/*
 * 初始化默认进程（硬编码 5 个示例进程）
 * 
 * 当 processes.txt 文件不存在时，用这 5 个默认进程来演示。
 * 你可以修改这些数值来测试不同的调度场景。
 */
void init_processes(struct pcb p[])
{
    strcpy(p[0].id, "P1");
    p[0].arrive = 0;
    p[0].burst = 5;
    p[0].priority = 3;

    strcpy(p[1].id, "P2");
    p[1].arrive = 2;
    p[1].burst = 2;
    p[1].priority = 5;

    strcpy(p[2].id, "P3");
    p[2].arrive = 4;
    p[2].burst = 3;
    p[2].priority = 1;

    strcpy(p[3].id, "P4");
    p[3].arrive = 5;
    p[3].burst = 4;
    p[3].priority = 4;

    strcpy(p[4].id, "P5");
    p[4].arrive = 6;
    p[4].burst = 1;
    p[4].priority = 2;
}

/*
 * 重置进程状态（每次运行调度算法之前调用）
 * 
 * 把进程的"运行时状态"字段恢复为初始值，这样每次调度都是独立运行的。
 * 相当于"洗牌重来"，确保不同调度算法之间互不影响。
 */
void reset_processes(struct pcb p[], int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        p[i].start = -1;             // -1 表示"还没开始运行"
        p[i].finish = -1;            // -1 表示"还没完成"
        p[i].remain = p[i].burst;    // 剩余运行时间 = 原始运行时间
        p[i].wait = 0;               // 等待时间清零
        p[i].level = 0;              // 队列级别归零（MLFQ 从最高级开始）
    }
}

/*
 * 复制进程数组（深拷贝每个结构体）
 * 
 * 调度算法会修改进程数据（start, finish, remain 等），
 * 所以需要先复制一份副本，在副本上操作，保护原始数据不被修改。
 * 参数：
 *   from[]  - 源数组
 *   to[]    - 目标数组
 *   count   - 进程数量
 */
void copy_processes(struct pcb from[], struct pcb to[], int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        to[i] = from[i];  // C 语言中结构体可以直接整体赋值，相当于逐字段复制
    }
}

/*
 * 按到达时间从小到大排序（插入排序算法）
 * 
 * 排序后，数组中的进程按 arrive 升序排列，
 * 方便后续调度算法按到达顺序处理。
 * 使用插入排序，因为进程数量少（N=10），简单高效。
 */
void sort_by_arrive(struct pcb p[], int count)
{
    int i, j;
    for (i = 1; i < count; i++)              // 从第二个元素开始，逐个插入
    {
        struct pcb temp = p[i];               // 暂存当前要插入的元素
        j = i - 1;
        // 在已排序部分中，从后往前找插入位置
        // 如果前面的元素到达时间更大，就把它往后移一位
        while (j >= 0 && p[j].arrive > temp.arrive)
        {
            p[j + 1] = p[j];                  // 后移
            j--;
        }
        p[j + 1] = temp;                      // 放入正确位置
    }
}

/*
 * 打印调度结果（表格形式）
 * 
 * 输出每个进程的调度结果，包括：
 *   - turn（周转时间）= 完成时间 - 到达时间（从到达到完成的总时间）
 *   - weight（带权周转时间）= 周转时间 / 运行时间（越小越好，>=1）
 * 最后还打印执行顺序，展示进程被调度的先后次序。
 */
void print_result(char *name, struct pcb p[], int count, char order[][10], int order_count)
{
    int i;
    printf("\n%s\n", name);
    printf("%-3s %6s %6s %6s %6s %5s %7s %8s\n",
           "id", "arrive", "burst", "start", "finish", "turn", "weight", "priority");
    for (i = 0; i < count; i++)
    {
        int turn = p[i].finish - p[i].arrive;     // 周转时间 = 完成 - 到达
        float weight = 0.0;
        if (p[i].burst != 0)                       // 防止除零错误
        {
            weight = (float)turn / p[i].burst;     // 带权周转时间
        }
        printf("%-3s %6d %6d %6d %6d %5d %7.2f %8d\n",
               p[i].id,
               p[i].arrive,
               p[i].burst,
               p[i].start,
               p[i].finish,
               turn,
               weight,
               p[i].priority);
    }
    printf("order: ");
    for (i = 0; i < order_count; i++)
    {
        printf("%s ", order[i]);                   // 打印执行顺序，如 "P1 P2 P3 P4 P5"
    }
    printf("\n");
}

/*
 * 打印当前进程列表（菜单选项 1 调用）
 * 
 * 只显示进程的基本属性：名称、到达时间、运行时间、优先级。
 * 不包含运行时状态（start, finish 等），因为这些在调度前是未知的。
 */
void print_process_list(struct pcb p[], int count)
{
    int i;
    printf("\nprocess list:\n");
    printf("%-3s %6s %6s %8s\n", "id", "arrive", "burst", "priority");
    for (i = 0; i < count; i++)
    {
        printf("%-3s %6d %6d %8d\n", p[i].id, p[i].arrive, p[i].burst, p[i].priority);
    }
    printf("\n");
}

/*
 * 从文件加载进程数据
 * 
 * 程序启动时调用，读取 processes.txt 文件中的进程数据。
 * 加载逻辑：
 *   1. 先尝试从 ../data/processes.txt 加载（相对路径）
 *   2. 如果失败，尝试从当前目录的 processes.txt 加载
 *   3. 如果都失败，调用 init_processes() 使用默认数据并保存到文件
 *   4. 如果文件内容为空（count==0），也使用默认数据
 * 
 * 返回值：进程数量
 */
int load_data(struct pcb p[])
{
    FILE *fp;
    int count = 0;
    int i;
    char file_name[20] = "processes.txt";

    fp = fopen("../data/processes.txt", "r");  // 尝试路径1
    if (fp == NULL)
    {
        fp = fopen(file_name, "r");             // 尝试路径2：当前目录
    }

    if (fp == NULL)                             // 两个路径都失败，文件不存在
    {
        init_processes(p);                      // 使用默认的 5 个进程
        save_data(p, 5);                        // 保存到文件，下次启动就能加载了
        return 5;
    }

    // 文件存在，读取数据
    fscanf(fp, "%d", &count);                   // 第一行：进程数量
    for (i = 0; i < count && i < N; i++)        // 最多读 N 个进程
    {
        fscanf(fp, "%s %d %d %d",               // 格式：名称 到达时间 运行时间 优先级
               p[i].id, &p[i].arrive, &p[i].burst, &p[i].priority);
    }
    fclose(fp);

    if (count == 0)                             // 文件内容为空
    {
        init_processes(p);
        count = 5;
        save_data(p, count);
    }

    return count;
}

/*
 * 保存进程数据到文件
 * 
 * 把当前的进程列表写入 processes.txt，下次启动时会自动加载。
 * 保存格式：
 *   第一行：进程数量
 *   后续每行：进程名称 到达时间 运行时间 优先级
 * 保存逻辑与 load_data 对应，先尝试 ../data/ 路径，再尝试当前目录。
 */
void save_data(struct pcb p[], int count)
{
    FILE *fp;
    int i;
    char file_name[20] = "processes.txt";

    fp = fopen("../data/processes.txt", "w");   // 尝试路径1（写模式）
    if (fp == NULL)
    {
        fp = fopen(file_name, "w");              // 尝试路径2：当前目录
    }

    if (fp == NULL)                              // 两个路径都打不开（如权限问题）
    {
        printf("can not save file\n");
        return;
    }

    fprintf(fp, "%d\n", count);                  // 第一行写进程数量
    for (i = 0; i < count; i++)
    {
        fprintf(fp, "%s %d %d %d\n",             // 每行写一个进程的四个属性
                p[i].id, p[i].arrive, p[i].burst, p[i].priority);
    }
    fclose(fp);
}

/*
 * 手动添加进程（菜单选项 6 调用）
 * 
 * 用户通过键盘输入进程的属性，添加到进程列表末尾。
 * 注意 count 是指针传递（int *count），因为需要修改外部变量的值。
 * 添加后会自动保存到文件。
 */
void add_process(struct pcb p[], int *count)
{
    char id[10];
    int arrive;
    int burst;
    int priority;

    printf("input process id: ");        // 输入进程名称，如 "P6"
    scanf("%s", id);
    printf("input arrive time: ");       // 输入到达时间
    scanf("%d", &arrive);
    printf("input burst time: ");        // 输入运行时间
    scanf("%d", &burst);
    printf("input priority: ");          // 输入优先级（数字越小优先级越高）
    scanf("%d", &priority);

    if (*count < N)                      // N=10，最多10个进程
    {
        strcpy(p[*count].id, id);        // 把新进程放到数组末尾
        p[*count].arrive = arrive;
        p[*count].burst = burst;
        p[*count].priority = priority;
        *count = *count + 1;             // 进程总数 +1（通过指针修改外部变量）
        printf("process added\n");
    }
    else
    {
        printf("too many processes\n");  // 超过最大容量
    }
}

/*
 * 随机生成进程（菜单选项 7 调用）
 * 
 * 用户输入进程数量，程序自动生成随机数据：
 *   - 到达时间：0 ~ 10 之间的随机数
 *   - 运行时间：1 ~ 8  之间的随机数
 *   - 优先级：  1 ~ 5  之间的随机数
 *   - 进程名：  自动命名为 P1, P2, P3, ...
 * 
 * srand(time(NULL)) 在 main 中已调用，确保每次运行结果不同。
 */
void random_processes(struct pcb p[], int *count)
{
    int num;
    int i;

    printf("how many processes to generate (1-%d): ", N);
    scanf("%d", &num);

    if (num < 1 || num > N)              // 检查数量是否合法
    {
        printf("invalid number, must be 1-%d\n", N);
        return;
    }

    for (i = 0; i < num; i++)
    {
        sprintf(p[i].id, "P%d", i + 1);           // 自动命名：P1, P2, P3...
        p[i].arrive   = rand() % 11;              // 到达时间：0 ~ 10
        p[i].burst    = rand() % 8 + 1;           // 运行时间：1 ~ 8
        p[i].priority = rand() % 5 + 1;           // 优先级：  1 ~ 5
    }

    *count = num;                                  // 更新进程总数
    printf("generated %d random processes\n", num);
    print_process_list(p, *count);                 // 显示生成结果
}

/*
 * FCFS 调度算法（First Come First Served，先来先服务）
 * 
 * 核心思想：谁先到达，谁先运行。就像排队买东西一样，先到的人先被服务。
 * 这是一个"非抢占式"算法，意味着一个进程一旦开始运行，就会一直运行到结束，
 * 中间不会被其他进程打断。
 * 
 * 参数说明：
 *   p[]     - 原始进程数组（不会被修改）
 *   count   - 进程总数
 */
void fcfs_run(struct pcb p[], int count)
{
    // ---- 第一步：准备工作 ----
    struct pcb jobs[N];            // 存放进程副本，避免修改原始数据
    int ready_index[10];           // 就绪队列：存放"已到达、等待运行"的进程在 jobs 中的下标
    int ready_count = 0;           // 就绪队列中当前有多少个进程
    int current_time = 0;          // 当前时间（模拟 CPU 时钟，从 0 开始）
    int index = 0;                 // 指向"下一个将要到达"的进程（因为 jobs 已按到达时间排序）
    int done = 0;                  // 已经完成运行的进程数量
    char order[MAX_ORDER][10];     // 记录执行顺序（哪个进程先运行、哪个后运行）
    int order_count = 0;           // 执行顺序数组中的条目数
    int i;

    // 复制一份进程数据，这样不会影响原始数据
    copy_processes(p, jobs, count);
    // 按照到达时间从小到大排序，方便后续按顺序处理
    sort_by_arrive(jobs, count);

    // ---- 第二步：主循环，一个一个地执行进程 ----
    while (done < count)  // 当还有进程没完成时，继续循环
    {
        // 2.1 把"当前时间之前已经到达"的进程加入就绪队列
        //     例如：current_time=3，那么到达时间<=3的进程都该进入就绪队列
        while (index < count && jobs[index].arrive <= current_time)
        {
            ready_index[ready_count] = index;  // 把进程的下标放入就绪队列
            ready_count++;                      // 就绪队列长度 +1
            index++;                            // 继续检查下一个进程
        }

        // 2.2 如果就绪队列为空，说明 CPU 空闲，没有进程可运行
        //     这时把时间快进到下一个进程的到达时间
        if (ready_count == 0)
        {
            current_time = jobs[index].arrive;  // 跳到下一个进程到达的时刻
            continue;                            // 重新循环，把新到达的进程加入就绪队列
        }

        // 2.3 FCFS 的核心：选择就绪队列中"第一个"进程（即最早到达的）
        //     ready_index[0] 就是最早进入队列的进程
        int choose = ready_index[0];

        // 2.4 把选中的进程从就绪队列中移除（队列头部出队）
        //     后面的元素依次向前移动一位
        for (i = 0; i < ready_count - 1; i++)
        {
            ready_index[i] = ready_index[i + 1];
        }
        ready_count--;  // 队列长度 -1

        // 2.5 记录进程的"开始运行时间"（只记录第一次开始运行的时间）
        if (jobs[choose].start < 0)  // start 初始为 -1，<0 说明还没开始过
        {
            jobs[choose].start = current_time;
        }

        // 2.6 记录执行顺序
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        // 2.7 FCFS 是非抢占式：进程一旦运行，就会一直运行到结束
        //     所以直接把当前时间加上该进程的剩余运行时间
        current_time += jobs[choose].remain;
        jobs[choose].finish = current_time;   // 记录完成时间
        jobs[choose].remain = 0;              // 剩余运行时间归零（进程已完成）
        done++;                                // 完成进程数 +1
    }

    // ---- 第三步：输出结果 ----
    print_result("FCFS", jobs, count, order, order_count);
}

/*
 * HRRN 调度算法（Highest Response Ratio Next，高响应比优先）
 * 
 * 核心思想：每次选择"响应比"最高的进程来运行。
 * 响应比 = (等待时间 + 运行时间) / 运行时间 = 1 + 等待时间/运行时间
 * 
 * 这个算法兼顾了公平和效率：
 *   - 等待越久的进程，响应比越大，越容易被选中（避免饥饿）
 *   - 运行时间短的进程，响应比增长快，也容易被选中（提高效率）
 * 
 * 也是非抢占式：选中后运行到结束。
 */
void hrrn_run(struct pcb p[], int count)
{
    struct pcb jobs[N];
    int ready_index[10];           // 就绪队列
    int ready_count = 0;
    int current_time = 0;
    int index = 0;
    int done = 0;
    char order[MAX_ORDER][10];
    int order_count = 0;
    int i;

    copy_processes(p, jobs, count);
    sort_by_arrive(jobs, count);

    while (done < count)
    {
        // 把已到达的进程加入就绪队列
        while (index < count && jobs[index].arrive <= current_time)
        {
            jobs[index].wait = 0;              // 初始化等待时间为 0
            ready_index[ready_count] = index;
            ready_count++;
            index++;
        }

        // CPU 空闲，快进时间
        if (ready_count == 0)
        {
            current_time = jobs[index].arrive;
            continue;
        }

        // ★ HRRN 的核心：遍历就绪队列，计算每个进程的响应比，选最大的
        int choose = 0;
        float best_ratio = -1.0;               // 当前最高响应比（初始 -1，保证第一个就会更新）
        for (i = 0; i < ready_count; i++)
        {
            int pos = ready_index[i];
            // 计算等待时间 = 当前时间 - 到达时间
            jobs[pos].wait = current_time - jobs[pos].arrive;
            if (jobs[pos].wait < 0)            // 保险：等待时间不能为负
            {
                jobs[pos].wait = 0;
            }
            // 响应比 = (等待时间 + 剩余运行时间) / 剩余运行时间
            // 例如：等了3个单位，还要跑2个单位 → 响应比 = (3+2)/2 = 2.5
            float ratio = (float)(jobs[pos].wait + jobs[pos].remain) / jobs[pos].remain;
            if (ratio > best_ratio)            // 找到更大的响应比就更新
            {
                best_ratio = ratio;
                choose = pos;                  // 记住这个进程在 jobs 中的下标
            }
        }

        // 从就绪队列中移除选中的进程（因为它在队列中间，需要找到位置再移除）
        for (i = 0; i < ready_count; i++)
        {
            if (ready_index[i] == choose)
            {
                int j;
                for (j = i; j < ready_count - 1; j++)
                {
                    ready_index[j] = ready_index[j + 1];  // 后面的元素前移
                }
                ready_count--;
                break;
            }
        }

        // 记录开始时间
        if (jobs[choose].start < 0)
        {
            jobs[choose].start = current_time;
        }
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        // 非抢占：一次性运行到结束
        current_time += jobs[choose].remain;
        jobs[choose].finish = current_time;
        jobs[choose].remain = 0;
        done++;
    }

    print_result("HRRN", jobs, count, order, order_count);
}

/*
 * RR 调度算法（Round Robin，时间片轮转）
 * 
 * 核心思想：每个进程轮流运行一小段时间（时间片），用完时间片后如果还没完成，
 * 就排到队尾等待下一轮。就像几个人轮流用一台电脑，每人只能用固定时间。
 * 这是一个"抢占式"算法，进程运行时间片用完后会被强制切换。
 * 
 * 与 FCFS 的关键区别：
 *   - FCFS：一个进程运行到结束才切换（非抢占）
 *   - RR：每个进程最多运行 time_slice 时长，时间到了就换下一个（抢占）
 * 
 * 参数说明：
 *   p[]     - 原始进程数组（不会被修改）
 *   count   - 进程总数
 */
void rr_run(struct pcb p[], int count)
{
    // ---- 第一步：准备工作 ----
    struct pcb jobs[N];            // 存放进程副本，避免修改原始数据
    int ready_index[10];           // 就绪队列：存放"已到达、等待运行"的进程在 jobs 中的下标
    int ready_count = 0;           // 就绪队列中当前有多少个进程
    int current_time = 0;          // 当前时间（模拟 CPU 时钟，从 0 开始）
    int index = 0;                 // 指向"下一个将要到达"的进程（因为 jobs 已按到达时间排序）
    int done = 0;                  // 已经完成运行的进程数量
    char order[MAX_ORDER][10];     // 记录执行顺序（哪个进程先运行、哪个后运行）
    int order_count = 0;           // 执行顺序数组中的条目数
    int i;
    int time_slice = 2;            // 时间片大小 = 2 个单位时间（每个进程每次最多运行 2）

    // 复制一份进程数据，这样不会影响原始数据
    copy_processes(p, jobs, count);
    // 按照到达时间从小到大排序，方便后续按顺序处理
    sort_by_arrive(jobs, count);

    // ---- 第二步：主循环，轮转执行进程 ----
    while (done < count)  // 当还有进程没完成时，继续循环
    {
        // 2.1 把"当前时间之前已经到达"的进程加入就绪队列
        while (index < count && jobs[index].arrive <= current_time)
        {
            ready_index[ready_count] = index;  // 把进程的下标放入就绪队列
            ready_count++;                      // 就绪队列长度 +1
            index++;                            // 继续检查下一个进程
        }

        // 2.2 如果就绪队列为空，说明 CPU 空闲，没有进程可运行
        //     这时把时间快进到下一个进程的到达时间
        if (ready_count == 0)
        {
            current_time = jobs[index].arrive;  // 跳到下一个进程到达的时刻
            continue;                            // 重新循环，把新到达的进程加入就绪队列
        }

        // 2.3 从就绪队列头部取出一个进程来运行（先进先出）
        int choose = ready_index[0];

        // 2.4 把这个进程从就绪队列中移除（头部出队）
        for (i = 0; i < ready_count - 1; i++)
        {
            ready_index[i] = ready_index[i + 1];
        }
        ready_count--;

        // 2.5 记录进程的"开始运行时间"（只记录第一次开始运行的时间）
        if (jobs[choose].start < 0)  // start 初始为 -1，<0 说明还没开始过
        {
            jobs[choose].start = current_time;
        }

        // 2.6 记录执行顺序（同一个进程可能被记录多次，因为它可能被多次调度）
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        // 2.7 RR 的核心：计算本次实际运行时间
        //     实际运行时间 = min(时间片, 剩余运行时间)
        //     例如：时间片=2，剩余=5 → 本次运行2，剩3
        //           时间片=2，剩余=1 → 本次运行1，剩0（进程完成）
        int run_time = time_slice;               // 先假设运行一个完整的时间片
        if (run_time > jobs[choose].remain)      // 如果时间片比剩余时间还大
        {
            run_time = jobs[choose].remain;      // 就只运行剩余的时间（避免运行过头）
        }

        // 2.8 更新当前时间和进程的剩余运行时间
        current_time += run_time;                // 时间前进 run_time
        jobs[choose].remain -= run_time;         // 进程剩余运行时间减少 run_time

        // 2.9 在运行过程中，可能有新进程到达，把它们加入就绪队列
        while (index < count && jobs[index].arrive <= current_time)
        {
            ready_index[ready_count] = index;
            ready_count++;
            index++;
        }

        // 2.10 判断该进程是否已经完成
        if (jobs[choose].remain == 0)            // 剩余时间为 0，进程完成了
        {
            jobs[choose].finish = current_time;  // 记录完成时间
            done++;                               // 完成进程数 +1
        }
        else                                     // 还没完成，放回就绪队列尾部
        {
            // ★ 这是 RR 与 FCFS 最大的不同：没完成的进程重新排队
            ready_index[ready_count] = choose;   // 把进程下标放回队尾
            ready_count++;                        // 队列长度 +1
        }
    }

    // ---- 第三步：输出结果 ----
    print_result("RR", jobs, count, order, order_count);
}

/*
 * MLFQ 调度算法（Multi-Level Feedback Queue，多级反馈队列）
 * 
 * 这是最复杂的调度算法，也是现代操作系统实际使用的算法。
 * 
 * 核心思想：设置 3 个优先级不同的队列（Q0 > Q1 > Q2），
 * 进程在不同队列中享受不同的时间片，用完时间片就降级到下一级队列。
 * 
 * 队列设计：
 *   队列0（最高优先级）：时间片 = 1，新进程都先进入这里
 *   队列1（中优先级）  ：时间片 = 2
 *   队列2（最低优先级）：时间片 = 4，用完后如果还没完成，继续留在队列2
 * 
 * 调度规则：
 *   1. 总是优先运行高优先级队列中的进程
 *   2. 只有高优先级队列为空时，才运行低优先级队列
 *   3. 进程在队列0用完时间片→降级到队列1→再降级到队列2
 * 
 * 优点：短进程很快完成（在高优先级），长进程也不会饿死（在低优先级用大时间片）
 */
void multi_level_run(struct pcb p[], int count)
{
    struct pcb jobs[N];
    // ★ 三个独立的就绪队列，分别对应三个优先级
    int queue0[10];          // 最高优先级队列（时间片=1）
    int queue1[10];          // 中优先级队列（时间片=2）
    int queue2[10];          // 最低优先级队列（时间片=4）
    int count0 = 0;          // 队列0 中的进程数
    int count1 = 0;          // 队列1 中的进程数
    int count2 = 0;          // 队列2 中的进程数
    int current_time = 0;
    int index = 0;
    int done = 0;
    char order[MAX_ORDER][10];
    int order_count = 0;
    int i;
    int level;               // 当前选中的进程来自哪个队列

    copy_processes(p, jobs, count);
    sort_by_arrive(jobs, count);

    while (done < count)
    {
        // 新到达的进程始终进入最高优先级队列（queue0）
        while (index < count && jobs[index].arrive <= current_time)
        {
            queue0[count0] = index;
            count0++;
            index++;
        }

        // 三个队列全空 → CPU 空闲，快进时间
        if (count0 == 0 && count1 == 0 && count2 == 0)
        {
            current_time = jobs[index].arrive;
            continue;
        }

        // ★ MLFQ 的核心选择逻辑：优先级 queue0 > queue1 > queue2
        int choose = -1;
        if (count0 > 0)                              // 优先检查队列0
        {
            choose = queue0[0];                      // 取队头
            for (i = 0; i < count0 - 1; i++)         // 队头出队（前移）
            {
                queue0[i] = queue0[i + 1];
            }
            count0--;
            level = 0;                               // 标记来自队列0
        }
        else if (count1 > 0)                         // 队列0为空，检查队列1
        {
            choose = queue1[0];
            for (i = 0; i < count1 - 1; i++)
            {
                queue1[i] = queue1[i + 1];
            }
            count1--;
            level = 1;                               // 标记来自队列1
        }
        else                                         // 队列0和1都空，用队列2
        {
            choose = queue2[0];
            for (i = 0; i < count2 - 1; i++)
            {
                queue2[i] = queue2[i + 1];
            }
            count2--;
            level = 2;                               // 标记来自队列2
        }

        if (jobs[choose].start < 0)
        {
            jobs[choose].start = current_time;
        }
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        // 根据进程所在的队列级别，分配不同的时间片
        int run_time = 1;                            // 队列0：时间片=1
        if (level == 1)
        {
            run_time = 2;                            // 队列1：时间片=2
        }
        if (level == 2)
        {
            run_time = 4;                            // 队列2：时间片=4
        }
        if (run_time > jobs[choose].remain)          // 实际运行时间不超过剩余时间
        {
            run_time = jobs[choose].remain;
        }

        current_time += run_time;
        jobs[choose].remain -= run_time;

        // 运行期间新到达的进程进入队列0
        while (index < count && jobs[index].arrive <= current_time)
        {
            queue0[count0] = index;
            count0++;
            index++;
        }

        // 判断进程是否完成
        if (jobs[choose].remain == 0)
        {
            jobs[choose].finish = current_time;
            done++;
        }
        else if (level == 0)                         // 队列0没跑完 → 降级到队列1
        {
            queue1[count1] = choose;
            count1++;
        }
        else if (level == 1)                         // 队列1没跑完 → 降级到队列2
        {
            queue2[count2] = choose;
            count2++;
        }
        else                                         // 队列2没跑完 → 继续留在队列2
        {
            queue2[count2] = choose;
            count2++;
        }
    }

    print_result("MLFQ", jobs, count, order, order_count);
}

/*
 * 主函数：程序入口，显示菜单并响应用户选择
 * 
 * 程序流程：
 *   1. 从文件加载进程数据（或初始化默认数据）
 *   2. 循环显示菜单，等待用户选择
 *   3. 根据选择执行对应的调度算法或操作
 *   4. 选择"8"保存数据并退出
 */
int main(void)
{
    struct pcb process_list[N];            // 进程数组，最多 N 个进程
    int count;                              // 当前进程数量
    int choice;                             // 用户菜单选择

    count = load_data(process_list);        // 启动时加载进程数据
    srand((unsigned int)time(NULL));         // 初始化随机数种子（每次运行产生不同随机数）

    while (1)                               // 无限循环，直到用户选择退出
    {
        printf("\n===== process scheduling system =====\n");
        printf("1. show process list\n");   // 查看当前所有进程
        printf("2. FCFS\n");               // 先来先服务调度
        printf("3. HRRN\n");               // 高响应比优先调度
        printf("4. RR\n");                 // 时间片轮转调度
        printf("5. MLFQ\n");               // 多级反馈队列调度
        printf("6. add process\n");        // 手动添加进程
        printf("7. random generate\n");    // 随机生成进程
        printf("8. save and exit\n");      // 保存并退出
        printf("choose: ");
        scanf("%d", &choice);

        if (choice == 1)
        {
            print_process_list(process_list, count);   // 显示进程列表
        }
        else if (choice == 2)
        {
            reset_processes(process_list, count);       // 重置状态（每次调度前必须重置）
            fcfs_run(process_list, count);              // 运行 FCFS 调度
        }
        else if (choice == 3)
        {
            reset_processes(process_list, count);
            hrrn_run(process_list, count);              // 运行 HRRN 调度
        }
        else if (choice == 4)
        {
            reset_processes(process_list, count);
            rr_run(process_list, count);                // 运行 RR 调度
        }
        else if (choice == 5)
        {
            reset_processes(process_list, count);
            multi_level_run(process_list, count);       // 运行 MLFQ 调度
        }
        else if (choice == 6)
        {
            add_process(process_list, &count);          // 添加进程（注意 &count 传地址）
            save_data(process_list, count);             // 添加后自动保存
        }
        else if (choice == 7)
        {
            random_processes(process_list, &count);     // 随机生成进程
            save_data(process_list, count);             // 保存到文件
        }
        else if (choice == 8)
        {
            save_data(process_list, count);             // 退出前保存数据
            break;                                      // 跳出 while 循环，程序结束
        }
        else
        {
            printf("wrong choice\n");                   // 输入了无效选项
        }
    }

    return 0;
}