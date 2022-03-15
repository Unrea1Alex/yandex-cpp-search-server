#include <vector>
#include <iostream>
#include "search_server.h"

void RemoveDuplicates(SearchServer& search_server)
{
    auto duplicated_ids = search_server.GetDuplicatedIds();

    for(auto id : duplicated_ids)
    {
        std::cout << "Found duplicate document id " << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
