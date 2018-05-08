#include <functional>





namespace
{

    class mhj_impl
    {

    public:

        using args = struct { double *vals; size_t n; };
        using function = std::function < double(double *) > ;

    private:
        const int n_count;
        const double eps_proc, eps_opt;
        const double t1, t2;
        const double d;
        int i, count, k;
        double a_m, b_m;
        double s, ss, lam, a, b;
        double m, l, fl, fm;
        double* y1;
        double* y;
        double* x1;
        double* dl;
        int N;
        double *x;
        args current;
        function func;

    public:

        mhj_impl(args output, function func, double eps_proc, double eps_opt, int n_count)
            : N(output.n)
            , x(output.vals)
            , func(func)
            , n_count(n_count)
            , eps_proc(eps_proc)
            , eps_opt(eps_opt)
            , t1(0.618033988749894)
            , t2(1 - 0.618033988749894)
            , a_m(-1)
            , b_m(1)
            , d(1)
        {
            y = new double[N];
            x1 = new double[N];
            dl = new double[N];
            y1 = new double[N];
            
            for (i = 0; i < N; i++)   x[i] = (double) (rand() - RAND_MAX / 2) / (double) RAND_MAX;
            for (i = 0; i < N; i++)   y[i] = x[i];

            count = 0;
        }

        ~mhj_impl()
        {
            delete[] y;
            delete[] x1;
            delete[] dl;
            delete[] y1;
        }

        bool step()
        {
            count++;
            for (i = 0; i < N; i++)  x1[i] = x[i];
            //=======================================================
            //Одномерная оптимизация методом золотого сечения по координатным направлениям

            for (i = 0; i < N; i++) y1[i] = y[i];
            for (k = 0; k < N; k++)
            {
                a = a_m; b = b_m;
                l = a + t2*(b - a);
                m = a + t1*(b - a);

                y1[k] = (double) (y[k] + l);//*d;
                fl = func(y1);
                y1[k] = (double) (y[k] + m);//*d;	
                fm = func(y1);

                do
                {
                    if (fm < fl)
                    {
                        a = l;  l = m;  m = a + t1*(b - a); fl = fm;
                        y1[k] = (double) (y[k] + m);//*d;
                        fm = func(y1);
                    }
                    else
                    {
                        b = m;  m = l;  l = a + t2*(b - a); fm = fl;
                        y1[k] = (double) (y[k] + l);//*d;
                        fl = func(y1);

                    }

                } while ((b - a) > eps_opt);
                //=========================================================
                lam = (a + b) / 2;
                y[k] = (double) (y[k] + lam*d);
            }

            for (i = 0; i < N; i++)   x[i] = y[i];

            s = 0.; ss = 0;
            for (i = 0; i < N; i++)
            {
                s += (x1[i] - x[i])*(x1[i] - x[i]);
                ss += x[i] * x[i];
            }
            if ((s < ss*eps_proc) || (count > n_count)) return true;

            for (i = 0; i < N; i++)   dl[i] = x[i] - x1[i];

            a = 0; b = 1;
            l = a + t2*(b - a);
            m = a + t1*(b - a);
            //==========================================================
            //Одномерная оптимизация методом золотого сечения по направлению
            for (i = 0; i < N; i++)	y1[i] = (double) (x[i] + l*dl[i]);
            fl = func(y1);
            for (i = 0; i < N; i++)	y1[i] = (double) (x[i] + m*dl[i]);
            fm = func(y1);

            do
            {
                if (fm < fl)
                {
                    a = l;  l = m;  m = a + t1*(b - a); fl = fm;
                    for (i = 0; i < N; i++) y1[i] = (double) (x[i] + m*dl[i]);
                    fm = func(y1);
                }
                else
                {
                    b = m;  m = l;  l = a + t2*(b - a); fm = fl;
                    for (i = 0; i < N; i++) y1[i] = (double) (x[i] + l*dl[i]);
                    fl = func(y1);
                }
            } while ((b - a) > eps_opt);

            lam = (a + b) / 2;
            for (i = 0; i < N; i++)   y[i] = (double) (x[i] + lam*dl[i]);
            return false;
        }
    };
}




class mhj_method
{

public:

    using args = mhj_impl::args;
    using function = mhj_impl::function;

    static void mhj(function func, args output)
    {
        mhj_method m(func, output);
        while (!++m) {}
    }

private:

    function func;
    args current;
    bool complete;

    mhj_impl impl;

    void init();

public:

    mhj_method(function func, args output, double eps_proc = 1e-6, double eps_opt = 1e-6, int n_count = 10000)
        : func(func)
        , current(output)
        , complete(false)
        , impl(output, func, eps_proc, eps_opt, n_count)
    {
    }

    mhj_method& operator ++()
    {
        complete = impl.step();
        return *this;
    }

    args operator *() const
    {
        return current;
    }

    operator bool () const
    {
        return complete;
    }
};