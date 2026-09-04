void use(int);

void withLambda(int n) {
    auto f = [n](int x) {
        int y = x + n;
        if (y > 0) {
            use(y);
        }
        return y;
    };
    f(1);
}

void immediatelyInvoked() {
    int r = [] { return 42; }();
    use(r);
}

void nested() {
    auto outer = [] {
        auto inner = [] { return 1; };
        return inner();
    };
    outer();
}
