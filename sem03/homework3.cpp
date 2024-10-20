#include <iostream>
#include <vector>
#include "papi.h"

class CSR_graph {
public:
    int row_count; //number of vertices in graph
    unsigned int col_count; //number of edges in graph
    
    std::vector<unsigned int> row_ptr; //index EDGES
    std::vector<int> col_ids;
    std::vector<double> vals;

    void read(const char* filename) {
        FILE *graph_file = fopen(filename, "rb");
        fread(reinterpret_cast<char*>(&row_count), sizeof(int), 1, graph_file);
        fread(reinterpret_cast<char*>(&col_count), sizeof(unsigned int), 1, graph_file);

        std::cout << "Row_count = " << row_count << ", col_count = " << col_count << std::endl;
        
        row_ptr.resize(row_count + 1);
        col_ids.resize(col_count);
        vals.resize(col_count);
         
        fread(reinterpret_cast<char*>(row_ptr.data()), sizeof(unsigned int), row_count + 1, graph_file);
        fread(reinterpret_cast<char*>(col_ids.data()), sizeof(int), col_count, graph_file);
        fread(reinterpret_cast<char*>(vals.data()), sizeof(double), col_count, graph_file);
        fclose(graph_file);
    }

    void print_vertex(int idx) {
        for (int col = row_ptr[idx]; col < row_ptr[idx + 1]; col++) {
            std::cout << col_ids[col] << " " << vals[col] <<std::endl;
        }
        std::cout << std::endl;
    }

    void reset() {
        row_count = 0;
        col_count = 0;
        row_ptr.clear();
        col_ids.clear();
        vals.clear();
    }
}; 

#define N_TESTS 5

int main () {
    const char* filenames[N_TESTS];
    filenames[0] = "synt";
    filenames[1] = "road_graph";
    filenames[2] = "stanford";
    filenames[3] = "youtube";
    filenames[4] = "syn_rmat";

    PAPI_library_init(7);
    int eventset = PAPI_NULL;
    PAPI_create_eventset(&eventset);
    PAPI_add_event(eventset, PAPI_L1_TCM);

    
    /* https://drive.google.com/file/d/183OMIj56zhqN12Aui1kxv76_Ia0vTPIF/view?usp=sharing архив с тестами, 
        распаковать командой tar -xzf 
    */

    for (int n_test = 0; n_test < N_TESTS; n_test++) {
        CSR_graph a;
        a.read(filenames[n_test]);
        long long info[1];


/*----------------------------<TASK 1>----------------------------*/
        PAPI_start(eventset);

        double max_weight = 0;
        double max_vertex = -1;
        for (int row = 0; row < a.row_count; row++) {
            int start_i = a.row_ptr[row];
            int end_i = a.row_ptr[row + 1];
            double sum = 0;
            for (int i = start_i; i < end_i; i++) {
                int dest = a.col_ids[i];
                double weight = a.vals[i];
                if (dest % 2 == 0) {
                    sum += weight;
                }
            }
            if (max_vertex != -1) {
                if (max_weight < sum) {
                    max_weight = sum;
                    max_vertex = row;
                }
            }
            else {
                max_weight = sum;
                max_vertex = row;
            }
        }
        std::cout << "max_vertex_sum: " << max_vertex << std::endl;
        std::cout << "PAPI_read_sum: " << PAPI_read(eventset, info) << std::endl;

        PAPI_stop(eventset, NULL);

/*----------------------------<TASK 2>----------------------------*/
        PAPI_start(eventset);

        int max_rank = 0;
        int max_vert = -1;
        for (int row = 0; row < a.row_count; row++) {
            int start_i = a.row_ptr[row];
            int end_i = a.row_ptr[row + 1];
            double rank = 0;
            for (int i = start_i; i < end_i; i++) {
                int dest = a.col_ids[i];
                double weight_i = a.vals[i];
                double sum = 0;
                int start_j = a.row_ptr[dest];
                int end_j = a.row_ptr[dest + 1];
                
                for (int j = start_j; j < end_j; j++) {
                    double weight_j = a.vals[j];
                    int vert = a.col_ids[j];
                    int count_inc_edges = a.row_ptr[vert + 1] - a.row_ptr[vert];
                    sum += weight_j * count_inc_edges;
                }
                rank += weight_i * sum;
            }
            if (max_vert != -1) {
                if (max_rank < rank) {
                    max_rank = rank;
                    max_vert = row;
                }
            }
            else {
                max_rank = rank;
                max_vert = row;
            }
        }
        std::cout << "max_vertex_rank: " << max_vert << std::endl;
        std::cout << "PAPI_read_rank: " << PAPI_read(eventset, info) << std::endl;

        PAPI_stop(eventset, NULL);
/*----------------------------------------------------------------*/
        
        a.reset();
    }

    PAPI_cleanup_eventset(eventset);
    PAPI_destroy_eventset(&eventset);
    // alg1    alg2
    // 447     2944
    // 474251  1379906
    // 28226   12252
    // 382     955277
    // 20486   6931

}