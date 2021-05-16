#include <unistd.h>
#include <stdlib.h>

double abs(double x)
{
    return x < 0 ? -x : x;
}

double f(double x)
{
    return x * x * x;
}

double integ(double left, double right, double step)
{
    double left_f = f(left), res = 0.0;
    for (left += step; left < right; left += step)
    {
        double right_f = f(left);
        res += (left_f + right_f) / 2.0 * step;
        left_f = right_f;
    }
    return res;
}

const double eps = 1e-5;

int main(void)
{
    sleep(10);
    uint64 start_time = get_time();
    double res = integ(0.0, 1.0, 1e-6);
    assert(abs(res - (1.0 / 4.0)) < eps, -33);
    return get_time() - start_time;
}
