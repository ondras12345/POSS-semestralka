#include <unity.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <PID.h>


FILE* prep_outfile(const char * name)
{
    FILE * fo = fopen(name, "w");
    TEST_ASSERT_NOT_NULL(fo);
    fprintf(fo, "k\tu\ty\tdu\tw\n");
    return fo;
}

#define Ts 0.01
#define N_SIM ((size_t)(15/Ts))
void test_PID()
{
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

    FILE * fo = prep_outfile("test_PID.csv");

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
        fprintf(fo, "%zu\t%e\t%e\t%e\t%e\n", k, u[k], y[k], du, w);
    }

    fclose(fo);

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


void test_PID_time()
{
    PID_t pid;
    PID_init(&pid, Ts);
    pid.Kp = 33;
    pid.Ki = 38;
    pid.Kd = 6.6;
    pid.Tf = 0.04;
    pid.b = 0.6;
    pid.c = 0.1;
    pid.umax = 50e3;
    pid.Tt = 5;
    PID_new_params(&pid);

    for (unsigned i = 0; i < 10; i++)
    {
        clock_t begin = clock();
        for (unsigned k = 0; k < 1000000U; k++)
        {
            PID_loop(&pid, 1e-4, 0.1f*k);
        }
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("time_spent=%lf\n", time_spent);
    }
}


void test_PID_angle_wrap()
{
    TEST_ASSERT_EQUAL_FLOAT(0, PID_angle_wrap(0));
    TEST_ASSERT_EQUAL_FLOAT(179, PID_angle_wrap(179));
    TEST_ASSERT_EQUAL_FLOAT(-179, PID_angle_wrap(-179));
    TEST_ASSERT_EQUAL_FLOAT(-90, PID_angle_wrap(270));
    TEST_ASSERT_EQUAL_FLOAT(90, PID_angle_wrap(-270));
    TEST_ASSERT_EQUAL_FLOAT(-90, PID_angle_wrap(990));
}


void test_PID_angle_loop()
{
    PID_angle_t pid;
    PID_angle_init(&pid, Ts);

    pid.Kp = 7.04;
    pid.Ki = 6.17;
    pid.Kd = 1.86;
    pid.Tf = 0.0433;
    pid.umax = 200;
    pid.Tt = sqrt(pid.Kd / pid.Ki);
    PID_angle_new_params(&pid);

    double y[N_SIM] = {0};
    double u[N_SIM] = {0};

    FILE * fo = prep_outfile("test_PID_angle.csv");

    for (size_t k = 0; k < N_SIM; k++)
    {
        double w = 0;
        if (k >= 150) w = 70.0;
        if (k > N_SIM*.25) w = 180.0;
        if (k > N_SIM*.5) w = 185.0;
        if (k > N_SIM*.66) w = -90.0;

        if (k >= 1)
        {
            // odezva systemu - integrator
            y[k] = y[k-1] + 0.5*Ts*u[k-1];

            if (k == 220)
            {
                y[k] = 0;
                PID_angle_reset(&pid, PID_angle_wrap(y[k]));
            }


            // vypocet PID
            u[k] = PID_angle_loop(&pid, PID_angle_wrap(y[k]), PID_angle_wrap(w));
        }

        float du = 0;
        fprintf(fo, "%zu\t%e\t%e\t%e\t%e\n", k, u[k], y[k], du, w);
    }

    fclose(fo);

    // see if angle_wrap works
    TEST_ASSERT_FLOAT_WITHIN(0.5, y[N_SIM-1], 270);
    // see if reset worked - the step response must be the same
    TEST_ASSERT_FLOAT_WITHIN(1e-8, y[150], y[220]);
    TEST_ASSERT_FLOAT_WITHIN(1e-8, y[151], y[221]);
    TEST_ASSERT_FLOAT_WITHIN(1e-8, y[172], y[242]);
}


int runUnityTests(void)
{
    if (chdir("test/test_PID"))
    {
        fprintf(stderr, "failed to chdir");
        return 1;
    }

    UNITY_BEGIN();
    RUN_TEST(test_PID);
    RUN_TEST(test_PID_time);
    RUN_TEST(test_PID_angle_wrap);
    RUN_TEST(test_PID_angle_loop);

    // gnuplot
    fprintf(stderr, "gnuplot result: %d\n", system("./test_PID.gpi"));

    return UNITY_END();
}


int main(void)
{
    return runUnityTests();
}
