/**
 * Backend loading testing.
 */

#include <knp/framework/backend_loader.h>

#include <tests_common.h>

#include <filesystem>


std::filesystem::path get_backend_path()
{
    return knp::testing::get_exe_path().parent_path() / "lib" / "knp-cpu-single-threaded-backend";
}


TEST(FrameworkSuite, BackendLoaderLoad)
{
    knp::framework::BackendLoader backend_loader;
    auto cpu_st_backend = backend_loader.load(get_backend_path());

    EXPECT_NO_THROW((void)cpu_st_backend->get_uid());
}


TEST(FrameworkSuite, BackendLoaderCheck)
{
    knp::framework::BackendLoader backend_loader;

    ASSERT_TRUE(backend_loader.is_backend(get_backend_path()));
}
