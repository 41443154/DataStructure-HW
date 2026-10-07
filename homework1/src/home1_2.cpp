#include "header.h"

using namespace std;

long long Ackermann(int m, long long n)
{
    const int MAX_SIZE = 1000000;

    int* s = new int[MAX_SIZE];
    int top = 0;

    s[top++] = m;

    while (top > 0)
    {
        m = s[--top];

        if (m == 0)
        {
            n = n + 1;
        }
        else if (n == 0)
        {
            if (top >= MAX_SIZE)
            {
                cout << "Stack overflow" << endl;
                delete[] s;
                exit(1);
            }

            s[top++] = m - 1;
            n = 1;
        }
        else
        {
            if (top + 2 >= MAX_SIZE)
            {
                cout << "Stack overflow" << endl;
                delete[] s;
                exit(1);
            }

            s[top++] = m - 1;
            s[top++] = m;
            n = n - 1;
        }
    }

    delete[] s;

    return n;
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