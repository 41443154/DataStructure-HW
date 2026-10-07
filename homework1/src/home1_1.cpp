#include "header.h"

using namespace std;

long long Ackermann(int m, long long n)
{
    if (m == 0)
    {
        return n + 1;
    }
    else if (n == 0)
    {
        return Ackermann(m - 1, 1);
    }
    else
    {
        return Ackermann(m - 1, Ackermann(m, n - 1));
    }
}

int main()
{
    int m;
    long long n;

    cout << "Input m n: ";
    cin >> m >> n;

    if (m < 0 || n < 0)
    {
        cout << "m and n must be >= 0" << endl;
        return 0;
    }

    cout << "A(" << m << ", " << n << ") = "
         << Ackermann(m, n) << endl;

    return 0;
}