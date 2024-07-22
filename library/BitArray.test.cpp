#include "BitArray.h"
#include "df/job_list_link.h"
#include <gtest/gtest.h>

class DFLinkedListTest : public testing::Test {
protected:
    DFLinkedListTest() {}
    df::job_list_link list;
};

TEST_F(DFLinkedListTest, edit){
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
    EXPECT_EQ(list.begin(), list.end());

    auto *job1 = new df::job();
    job1->id = 1;

    auto *job2 = new df::job();
    job2->id = 2;

    auto *job3 = new df::job();
    job3->id = 3;


    list.push_front(job1); // { 1 }
    EXPECT_EQ(list.next->prev, &list);

    auto it = list.begin();
    EXPECT_EQ((*it)->id, 1);
    EXPECT_EQ(job1->list_link->item, job1);

    it = list.insert_after(it,job2); // { 1 2 }
    EXPECT_EQ((*it)->id,2);
    EXPECT_EQ(job2->list_link->item, job2);

    it = list.insert(it,job3); // { 1 3 2 }
    EXPECT_EQ((*it)->id,3);
    EXPECT_EQ(job3->list_link->item, job3);

    ++it;
    EXPECT_EQ(*it, job2);
    --it;
    EXPECT_EQ(*it, job3);

    it = list.erase(it); // { 1 2 }
    EXPECT_EQ(job3->list_link, nullptr);
    EXPECT_EQ(*it, job2);
    EXPECT_EQ(list.size(), 2);

    it = list.end();
    EXPECT_THROW(list.insert_after(it,job3),DFHack::Error::InvalidArgument);
    --it; // linear search for end()-1
    EXPECT_EQ(*it, job2);

    while (!list.empty())
    {
        list.erase(list.begin());
    }
    EXPECT_EQ(list.size(), 0);
    EXPECT_EQ(job2->list_link, nullptr);
    EXPECT_EQ(job1->list_link, nullptr);

    delete job1;
    delete job2;
    delete job3;
}
