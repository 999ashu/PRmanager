// #include <greeting.hpp>

// #include <userver/utest/utest.hpp>

// using PRmanager::UserType;

// UTEST(SayHelloTo, Basic) {
//     EXPECT_EQ(PRmanager::SayHelloTo("Developer", UserType::kFirstTime), "Hello, Developer!\n");
//     EXPECT_EQ(PRmanager::SayHelloTo({}, UserType::kFirstTime), "Hello, unknown user!\n");

//     EXPECT_EQ(PRmanager::SayHelloTo("Developer", UserType::kKnown), "Hi again, Developer!\n");
//     EXPECT_EQ(PRmanager::SayHelloTo({}, UserType::kKnown), "Hi again, unknown user!\n");
// }