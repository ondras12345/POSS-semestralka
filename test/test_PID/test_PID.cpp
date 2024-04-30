#include <unity.h>

#include <stdio.h>
#include <unistd.h>
#include <PID.h>


void test_PID()
{
#define Ts 0.01
#define N_SIM ((size_t)(15/Ts))
    PID_t pid;
    PID_init(&pid, Ts);

    double n[] = { 0.0, 0.498171282423739e-4, 0.496348000141384e-4 };
    double d[] = { 1.0, -1.989050333582543, 0.989060278775369 };

    pid.Kp = 33;
    pid.Ki = 38;
    pid.Kd = 6.6;
    pid.Tf = 0.04;
    pid.b = 0.6;
    pid.c = 0.0;
    pid.umax = 50;
    pid.Tt = 5;
    PID_new_params(&pid);

    double y[N_SIM] = {0};
    double u[N_SIM] = {0};

    TEST_ASSERT_FALSE_MESSAGE(chdir("test/test_PID"), "failed to chdir");

    FILE * fo = fopen("test_PID.csv", "w");
    TEST_ASSERT_NOT_NULL(fo);

    fprintf(fo, "k\tu\ty\tdu\n");

    for (size_t k = 0; k < N_SIM; k++)
    {
        double du = 0;
        if (k >= 10/Ts-1) du = 30;

        double w = 0;
        if (k >= 5/Ts-1) w = 10;

        if (k >= 2)
        {
            // odezva systemu
            y[k] = -d[1]*y[k-1] -d[2]*y[k-2] +n[1]*u[k-1] +n[2]*u[k-2];

            // zmena parametru
            if (k == 8/Ts)
            {
                pid.Kp = 50;
                pid.b = 1.0;
                //pid.Kd = 12;
                pid.c = 1.0;
                PID_new_params(&pid);
            }

            // vypocet PID
            u[k] = PID_loop(&pid, y[k], w);
        }

        // pridani vstupni poruchy
        u[k] += du;

        // vypis
        fprintf(fo, "%zu\t%e\t%e\t%e\n", k, u[k], y[k], du);
    }

    fclose(fo);

    // gnuplot
    fprintf(stderr, "gnuplot result: %d\n", system("./test_PID.gpi"));

    FILE * fi = fopen("test_PID_MATLAB.csv", "r");
    TEST_ASSERT_NOT_NULL(fi);

    // drop first line
    for (;;)
    {
        int c = fgetc(fi);
        if (c == '\n' || c == EOF) break;
    }

    // compare results
    double y_matl[N_SIM] = {0};
    double u_matl[N_SIM] = {0};
    size_t k;
    for (k = 0; k < N_SIM; k++)
    {
        unsigned k_csv;
        double du_matl;
        if (fscanf(fi, "%u %lf %lf %lf", &k_csv, &u_matl[k], &y_matl[k], &du_matl) != 4) break;
        TEST_ASSERT_EQUAL_UINT(k, k_csv);
    }
    TEST_ASSERT_EQUAL_size_t(N_SIM, k);

    fclose(fi);


    // This version of unity does not support TEST_ASSERT_DOUBLE_ARRAY_WITHIN
    for (k = 0; k < N_SIM; k++)
    {
        char str[50];
        snprintf(str, sizeof str, "k=%zu", k);
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-3, u_matl[k], u[k], str);
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-3, y_matl[k], y[k], str);
    }
}


int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_PID);
    return UNITY_END();
}


int main(void)
{
    return runUnityTests();
}
