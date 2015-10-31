const unsigned char *w_cur = w_chunk_base;
unsigned char *output_base0 = output + ((x0+0)*nOutputPlanes + oi0*OP_BLOCK_SIZE)*sizeof(float);

vreg_t oreg00 = zero_vreg();
vreg_t oreg01 = zero_vreg();

for (int dposx=0; dposx<3; dposx++) {
    int dposx2 = dposx-1;

    if ((x0 == 0) && (dposx == 0)) {
        dposx2 = 0;
    }
    if ((x0 == width-1) && (dposx == 2)) {
        dposx2 = 0;
    }

    const unsigned char *input_cur_x0 = in + ((dposy2*width + x0 + dposx2)*nInputPlanes+ii0*IP_BLOCK_SIZE)*sizeof(float);

    /* load4, madd4 */
#define MUL_W_IN(I)                                                     \
    {                                                                   \
        int I2 = I;                                                     \
        vreg_t wreg0, wreg1;                                            \
        vreg_t ireg0;                                                   \
                                                                        \
        wreg0 = load_vreg(w_cur);                                       \
        wreg1 = load_vreg(w_cur + VEC_NELEM*sizeof(float));             \
                                                                        \
        ireg0 = load_vreg_broadcast(input_cur_x0);                      \
        oreg00 = madd_vreg(wreg0, ireg0, oreg00);                       \
        oreg01 = madd_vreg(wreg1, ireg0, oreg01);                       \
                                                                        \
        w_cur += OP_BLOCK_SIZE * sizeof(float);                         \
        input_cur_x0 += sizeof(float);                                  \
    }

    for (int ii1=0; ii1<IP_BLOCK_SIZE; ii1+=4) {
        MUL_W_IN(ii1+0);
        MUL_W_IN(ii1+1);
        MUL_W_IN(ii1+2);
        MUL_W_IN(ii1+3);
    }
}

if (dposy == 0 && ii0 == 0) {
    store_vreg(output_base0 + (        0)*sizeof(float), oreg00);
    store_vreg(output_base0 + (VEC_NELEM)*sizeof(float), oreg01);
} else if (last_iter) {
    vreg_t tmp00, tmp01;

    vreg_t mtz, ltz, bv0, bv1;

    bv0 = load_vreg(biases + (oi0*OP_BLOCK_SIZE+        0)*sizeof(float));
    bv1 = load_vreg(biases + (oi0*OP_BLOCK_SIZE+VEC_NELEM)*sizeof(float));

#define ReLU(addr, bv, N)                               \
    tmp##N = load_vreg(addr);                           \
    tmp##N = add_vreg(tmp##N, oreg##N);                 \
    tmp##N = add_vreg(tmp##N, bv);                      \
    mtz = max_vreg(tmp##N, zero_vreg());                \
    ltz = min_vreg(tmp##N, zero_vreg());                \
    tmp##N = madd_vreg(ltz, set1_vreg(0.1f), mtz);      \

    ReLU(output_base0 + (        0)*sizeof(float), bv0, 00);
    ReLU(output_base0 + (VEC_NELEM)*sizeof(float), bv1, 01);

    store_vreg(output_base0 + (        0)*sizeof(float), tmp00);
    store_vreg(output_base0 + (VEC_NELEM)*sizeof(float), tmp01);
} else {
    vreg_t tmp;

#define ADD_TO_MEM(addr, val)                   \
    tmp = load_vreg(addr);                      \
    tmp = add_vreg(tmp, val);                   \
    store_vreg(addr, tmp);

    ADD_TO_MEM(output_base0 + (        0)*sizeof(float), oreg00);
    ADD_TO_MEM(output_base0 + (VEC_NELEM)*sizeof(float), oreg01);
}

#undef MUL_W_IN
#ifdef accumulate
#undef accumulate
#endif
#undef ADD_TO_MEM
#undef ReLU