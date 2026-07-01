#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define N 10
#define MAX_ORDER 10

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

void reset_processes(struct pcb p[], int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        p[i].start = -1;
        p[i].finish = -1;
        p[i].remain = p[i].burst;
        p[i].wait = 0;
        p[i].level = 0;
    }
}

void copy_processes(struct pcb from[], struct pcb to[], int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        to[i] = from[i];
    }
}

void sort_by_arrive(struct pcb p[], int count)
{
    int i, j;
    for (i = 1; i < count; i++)
    {
        struct pcb temp = p[i];
        j = i - 1;
        while (j >= 0 && p[j].arrive > temp.arrive)
        {
            p[j + 1] = p[j];
            j--;
        }
        p[j + 1] = temp;
    }
}

void print_result(char *name, struct pcb p[], int count, char order[][10], int order_count)
{
    int i;
    printf("\n%s\n", name);
    printf("id  arrive burst start finish turn weight priority\n");
    for (i = 0; i < count; i++)
    {
        int turn = p[i].finish - p[i].arrive;
        float weight = 0.0;
        if (p[i].burst != 0)
        {
            weight = (float)turn / p[i].burst;
        }
        printf("%s   %d     %d    %d    %d    %d   %.2f     %d\n",
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
        printf("%s ", order[i]);
    }
    printf("\n");
}

void print_process_list(struct pcb p[], int count)
{
    int i;
    printf("\nprocess list:\n");
    printf("id  arrive burst priority\n");
    for (i = 0; i < count; i++)
    {
        printf("%s   %d     %d    %d\n", p[i].id, p[i].arrive, p[i].burst, p[i].priority);
    }
}

int load_data(struct pcb p[])
{
    FILE *fp;
    int count = 0;
    int i;
    char file_name[20] = "processes.txt";

    fp = fopen("../data/processes.txt", "r");
    if (fp == NULL)
    {
        fp = fopen(file_name, "r");
    }

    if (fp == NULL)
    {
        init_processes(p);
        save_data(p, 5);
        return 5;
    }

    fscanf(fp, "%d", &count);
    for (i = 0; i < count && i < N; i++)
    {
        fscanf(fp, "%s %d %d %d", p[i].id, &p[i].arrive, &p[i].burst, &p[i].priority);
    }
    fclose(fp);

    if (count == 0)
    {
        init_processes(p);
        count = 5;
        save_data(p, count);
    }

    return count;
}

void save_data(struct pcb p[], int count)
{
    FILE *fp;
    int i;
    char file_name[20] = "processes.txt";

    fp = fopen("../data/processes.txt", "w");
    if (fp == NULL)
    {
        fp = fopen(file_name, "w");
    }

    if (fp == NULL)
    {
        printf("can not save file\n");
        return;
    }

    fprintf(fp, "%d\n", count);
    for (i = 0; i < count; i++)
    {
        fprintf(fp, "%s %d %d %d\n", p[i].id, p[i].arrive, p[i].burst, p[i].priority);
    }
    fclose(fp);
}

void add_process(struct pcb p[], int *count)
{
    char id[10];
    int arrive;
    int burst;
    int priority;

    printf("input process id: ");
    scanf("%s", id);
    printf("input arrive time: ");
    scanf("%d", &arrive);
    printf("input burst time: ");
    scanf("%d", &burst);
    printf("input priority: ");
    scanf("%d", &priority);

    if (*count < N)
    {
        strcpy(p[*count].id, id);
        p[*count].arrive = arrive;
        p[*count].burst = burst;
        p[*count].priority = priority;
        *count = *count + 1;
        printf("process added\n");
    }
    else
    {
        printf("too many processes\n");
    }
}

void fcfs_run(struct pcb p[], int count)
{
    struct pcb jobs[N];
    int ready_index[10];
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
        while (index < count && jobs[index].arrive <= current_time)
        {
            ready_index[ready_count] = index;
            ready_count++;
            index++;
        }

        if (ready_count == 0)
        {
            current_time = jobs[index].arrive;
            continue;
        }

        int choose = ready_index[0];
        for (i = 0; i < ready_count - 1; i++)
        {
            ready_index[i] = ready_index[i + 1];
        }
        ready_count--;

        if (jobs[choose].start < 0)
        {
            jobs[choose].start = current_time;
        }
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        current_time += jobs[choose].remain;
        jobs[choose].finish = current_time;
        jobs[choose].remain = 0;
        done++;
    }

    print_result("FCFS", jobs, count, order, order_count);
}

void hrrn_run(struct pcb p[], int count)
{
    struct pcb jobs[N];
    int ready_index[10];
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
        while (index < count && jobs[index].arrive <= current_time)
        {
            jobs[index].wait = 0;
            ready_index[ready_count] = index;
            ready_count++;
            index++;
        }

        if (ready_count == 0)
        {
            current_time = jobs[index].arrive;
            continue;
        }

        int choose = 0;
        float best_ratio = -1.0;
        for (i = 0; i < ready_count; i++)
        {
            int pos = ready_index[i];
            jobs[pos].wait = current_time - jobs[pos].arrive;
            if (jobs[pos].wait < 0)
            {
                jobs[pos].wait = 0;
            }
            float ratio = (float)(jobs[pos].wait + jobs[pos].remain) / jobs[pos].remain;
            if (ratio > best_ratio)
            {
                best_ratio = ratio;
                choose = pos;
            }
        }

        for (i = 0; i < ready_count; i++)
        {
            if (ready_index[i] == choose)
            {
                int j;
                for (j = i; j < ready_count - 1; j++)
                {
                    ready_index[j] = ready_index[j + 1];
                }
                ready_count--;
                break;
            }
        }

        if (jobs[choose].start < 0)
        {
            jobs[choose].start = current_time;
        }
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        current_time += jobs[choose].remain;
        jobs[choose].finish = current_time;
        jobs[choose].remain = 0;
        done++;
    }

    print_result("HRRN", jobs, count, order, order_count);
}

void rr_run(struct pcb p[], int count)
{
    struct pcb jobs[N];
    int ready_index[10];
    int ready_count = 0;
    int current_time = 0;
    int index = 0;
    int done = 0;
    char order[MAX_ORDER][10];
    int order_count = 0;
    int i;
    int time_slice = 2;

    copy_processes(p, jobs, count);
    sort_by_arrive(jobs, count);

    while (done < count)
    {
        while (index < count && jobs[index].arrive <= current_time)
        {
            ready_index[ready_count] = index;
            ready_count++;
            index++;
        }

        if (ready_count == 0)
        {
            current_time = jobs[index].arrive;
            continue;
        }

        int choose = ready_index[0];
        for (i = 0; i < ready_count - 1; i++)
        {
            ready_index[i] = ready_index[i + 1];
        }
        ready_count--;

        if (jobs[choose].start < 0)
        {
            jobs[choose].start = current_time;
        }
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        int run_time = time_slice;
        if (run_time > jobs[choose].remain)
        {
            run_time = jobs[choose].remain;
        }

        current_time += run_time;
        jobs[choose].remain -= run_time;

        while (index < count && jobs[index].arrive <= current_time)
        {
            ready_index[ready_count] = index;
            ready_count++;
            index++;
        }

        if (jobs[choose].remain == 0)
        {
            jobs[choose].finish = current_time;
            done++;
        }
        else
        {
            ready_index[ready_count] = choose;
            ready_count++;
        }
    }

    print_result("RR", jobs, count, order, order_count);
}

void multi_level_run(struct pcb p[], int count)
{
    struct pcb jobs[N];
    int queue0[10];
    int queue1[10];
    int queue2[10];
    int count0 = 0;
    int count1 = 0;
    int count2 = 0;
    int current_time = 0;
    int index = 0;
    int done = 0;
    char order[MAX_ORDER][10];
    int order_count = 0;
    int i;
    int level;

    copy_processes(p, jobs, count);
    sort_by_arrive(jobs, count);

    while (done < count)
    {
        while (index < count && jobs[index].arrive <= current_time)
        {
            queue0[count0] = index;
            count0++;
            index++;
        }

        if (count0 == 0 && count1 == 0 && count2 == 0)
        {
            current_time = jobs[index].arrive;
            continue;
        }

        int choose = -1;
        if (count0 > 0)
        {
            choose = queue0[0];
            for (i = 0; i < count0 - 1; i++)
            {
                queue0[i] = queue0[i + 1];
            }
            count0--;
            level = 0;
        }
        else if (count1 > 0)
        {
            choose = queue1[0];
            for (i = 0; i < count1 - 1; i++)
            {
                queue1[i] = queue1[i + 1];
            }
            count1--;
            level = 1;
        }
        else
        {
            choose = queue2[0];
            for (i = 0; i < count2 - 1; i++)
            {
                queue2[i] = queue2[i + 1];
            }
            count2--;
            level = 2;
        }

        if (jobs[choose].start < 0)
        {
            jobs[choose].start = current_time;
        }
        strcpy(order[order_count], jobs[choose].id);
        order_count++;

        int run_time = 1;
        if (level == 1)
        {
            run_time = 2;
        }
        if (level == 2)
        {
            run_time = 4;
        }
        if (run_time > jobs[choose].remain)
        {
            run_time = jobs[choose].remain;
        }

        current_time += run_time;
        jobs[choose].remain -= run_time;

        while (index < count && jobs[index].arrive <= current_time)
        {
            queue0[count0] = index;
            count0++;
            index++;
        }

        if (jobs[choose].remain == 0)
        {
            jobs[choose].finish = current_time;
            done++;
        }
        else if (level == 0)
        {
            queue1[count1] = choose;
            count1++;
        }
        else if (level == 1)
        {
            queue2[count2] = choose;
            count2++;
        }
        else
        {
            queue2[count2] = choose;
            count2++;
        }
    }

    print_result("MLFQ", jobs, count, order, order_count);
}

int main(void)
{
    struct pcb process_list[N];
    int count;
    int choice;

    count = load_data(process_list);

    while (1)
    {
        printf("\n===== process scheduling system =====\n");
        printf("1. show process list\n");
        printf("2. FCFS\n");
        printf("3. HRRN\n");
        printf("4. RR\n");
        printf("5. MLFQ\n");
        printf("6. add process\n");
        printf("7. save and exit\n");
        printf("choose: ");
        scanf("%d", &choice);

        if (choice == 1)
        {
            print_process_list(process_list, count);
        }
        else if (choice == 2)
        {
            reset_processes(process_list, count);
            fcfs_run(process_list, count);
        }
        else if (choice == 3)
        {
            reset_processes(process_list, count);
            hrrn_run(process_list, count);
        }
        else if (choice == 4)
        {
            reset_processes(process_list, count);
            rr_run(process_list, count);
        }
        else if (choice == 5)
        {
            reset_processes(process_list, count);
            multi_level_run(process_list, count);
        }
        else if (choice == 6)
        {
            add_process(process_list, &count);
            save_data(process_list, count);
        }
        else if (choice == 7)
        {
            save_data(process_list, count);
            break;
        }
        else
        {
            printf("wrong choice\n");
        }
    }

    return 0;
}
