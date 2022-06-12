SEARCH-ENGINE
Document search engine with negative keywords (documents with these words will not be displayed in the search results). It works in the likeness of a search engine, such as Yandex. The ranking of the results is based on TF-IDF.

Building and Run
  1. mkdir BuildSearchEngine && cd BuildSearchEngine
  2. cmake ..
  3. cmake --build .
  4. Start ./search_engine or search_engine.exe

System requirements and Stack
C++17
GCC version 8.1.0
Cmake 3.21.2 (minimal 3.10)
TF-IDF

Future plans
Add a user interface for adding documents and searching through them
