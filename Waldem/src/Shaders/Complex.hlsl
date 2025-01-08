struct complex
{
    float real;
    float imag;
};

complex ccreate(float real, float imag)
{
    complex result;
    result.real = real;
    result.imag = imag;
    return result;
}

complex cmul(complex c0, complex c1)
{
    complex result;
    result.real = c0.real * c1.real - c0.imag * c1.imag;
    result.imag = c0.real * c1.imag + c0.imag * c1.real;
    return result;
}

complex cadd(complex c0, complex c1)
{
    complex result;
    result.real = c0.real + c1.real;
    result.imag = c0.imag + c1.imag;
    return result;
}

complex cconj(complex c)
{
    complex result;
    result.real = c.real;
    result.imag = -c.imag;
    return result;
}