#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>
#include <pair.h>


//TODO Test with known result for serialized elements?

static void test_pairing(void **state){
 char * seed="Seed_test_pairing_0123456789";
    int seedLength=28;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z1=zpRandom(rng);
    Zp* z2=zpRandom(rng);
    Zp* z3=zpRandom(rng);
    Zp* z4=zpRandom(rng);
    G1* g1=g1Generator(); 
    G1* g2; 
    G2* g3=g2Generator(); 
    G2* g4;
    G3 *res1,*res2,*res3;
    g1Mul(g1,z1);    //g1=[z1]g
    g2Mul(g3,z3);    //g3=[z3]g
    g2=g1Copy(g1);   //g2=[z1]g'
    g4=g2Copy(g3);   //g4=[z3]g'
    //e([a]p,q)=e(p,q)^a
    g1Mul(g1,z2);   //g1=[z2]([z1]g)
    res1=pair(g1,g3);  //res1=e([z2]([z1]g),g3)
    res2=pair(g2,g3); 
    g3Exp(res2,z2);     //res2=e([z1]g,g3)^z2
    assert_true(g3equals(res1,res2));
    g3Free(res1);
    g3Free(res2);
    //e(p,[b]q)=e(p,q)^b
    g2Mul(g3,z4);
    res1=pair(g2,g3);
    res2=pair(g2,g4);
    g3Exp(res2,z4);
    assert_true(g3equals(res1,res2));
    g3Free(res1);
    g3Free(res2);
    //e(p1+p2,q)=e(p1,q)e(p2,q)
    res1=pair(g1,g3);
    res2=pair(g2,g3);
    g3Mul(res1,res2);
    g1Add(g1,g2);
    res3=pair(g1,g3);
    assert_true(g3equals(res1,res3));
    g3Free(res1);
    g3Free(res2);
    g3Free(res3);
    //e(p,q1+q2)=e(p,q1)e(p,q2)
    res1=pair(g1,g3);
    res2=pair(g1,g4);
    g3Mul(res1,res2);
    g2Add(g3,g4);
    res3=pair(g1,g3);
    assert_true(g3equals(res1,res3));
    zpFree(z1);
    zpFree(z2);
    zpFree(z3);
    zpFree(z4);
    g1Free(g1);
    g1Free(g2);
    g2Free(g3);
    g2Free(g4);
    g3Free(res1);
    g3Free(res2);
    g3Free(res3);
    rgFree(rng);
}

static void test_multipair(void **state)
{
    char * seed="Seed_test_multipair_0123456789";
    int seedLength=29;
    ranGen * rng=rgInit(seed,seedLength);
    Zp* z1=zpRandom(rng);
    Zp* z2=zpRandom(rng);
    Zp* z3=zpRandom(rng);
    Zp* z4=zpRandom(rng);
    G1* el1[2];
    G2* el2[2];
    G1* g1=g1Generator(); 
    G1* g2=g1Generator(); 
    G2* g3=g2Generator(); 
    G2* g4=g2Generator(); 
    G3 *res1,*res2,*res3;
    g1Mul(g1,z1);
    g1Mul(g2,z2);
    g2Mul(g3,z3);
    g2Mul(g4,z4);
    el1[0]=g1;
    el1[1]=g2;
    el2[0]=g3;
    el2[1]=g4;
    res1=pair(g1,g3);
    res2=pair(g2,g4);
    g3Mul(res1,res2);
    g3Free(res2);
    res2=doublepair(g1,g2,g3,g4);
    assert_true(g3equals(res1,res2));
    res3=multipair((const G1**) el1,(const G2**) el2,2);
    assert_true(g3equals(res2,res3));
    zpFree(z1);
    zpFree(z2);
    zpFree(z3);
    zpFree(z4);
    g1Free(g1);
    g1Free(g2);
    g2Free(g3);
    g2Free(g4);
    g3Free(res1);
    g3Free(res2);
    g3Free(res3);
    rgFree(rng);
}


int main()
{
    const struct CMUnitTest pairtests[] =
    {
        cmocka_unit_test(test_pairing),
        cmocka_unit_test(test_multipair)
    };
    //cmocka_set_message_output(CM_OUTPUT_XML);
	// Define environment variable CMOCKA_XML_FILE=testresults/libc.xml 
    return cmocka_run_group_tests(pairtests, NULL, NULL);
}