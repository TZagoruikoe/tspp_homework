#include <iostream>
#include <vector>
#include <papi.h>
#include <cstdio>

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

    int version = PAPI_library_init(PAPI_VER_CURRENT);
    if (version != PAPI_VER_CURRENT) {
        std::cout << "ERROR: Init. Version: " << version << std::endl;
    }

    int event_code;
    int check_event_code = PAPI_event_name_to_code("perf::CYCLES", &event_code);
    if (check_event_code != PAPI_OK) {
        std::cout << "ERROR: Event code --> " << check_event_code << std::endl;
    }

    int eventset = PAPI_NULL;
    if (PAPI_create_eventset(&eventset) != PAPI_OK) {
        std::cout << "ERROR: Create" << std::endl;
    }
    int check_add1 = PAPI_add_event(eventset, PAPI_L1_DCM);
    if (check_add1 != PAPI_OK) {
        std::cout << "ERROR: Add1 --> " << check_add1 << std::endl;
    }
    
    int check_add2 = PAPI_add_event(eventset, PAPI_L2_DCM);
    if (check_add2 != PAPI_OK) {
        std::cout << "ERROR: Add2 --> " << check_add2 << std::endl;
    }

    int check_add3 = PAPI_add_event(eventset, event_code);
    if (check_add3 != PAPI_OK) {
        std::cout << "ERROR: Add3 --> " << check_add3 << std::endl;
    }

    
    /* https://drive.google.com/file/d/183OMIj56zhqN12Aui1kxv76_Ia0vTPIF/view?usp=sharing архив с тестами, 
        распаковать командой tar -xzf 
    */

    for (int n_test = 0; n_test < N_TESTS; n_test++) {
        CSR_graph a;
        a.read(filenames[n_test]);
        long long info[3];


/*----------------------------<TASK 1>----------------------------*/
        if (PAPI_start(eventset) != PAPI_OK) {
            std::cout << "ERROR: Start" << std::endl;
        }

        double max_weight = 0;
        int max_vertex = -1;
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

        PAPI_stop(eventset, info);

        std::cout << "1PAPI_L1_DCM: " << info[0] << std::endl;
        std::cout << "1PAPI_L2_DCM: " << info[1] << std::endl;
        std::cout << "1PAPI native: " << info[2] << std::endl;

/*----------------------------<TASK 2>----------------------------*/
        PAPI_start(eventset);

        double max_rank = 0;
        int max_vert = -1;
        for (int row = 0; row < a.row_count; row++) {
            unsigned int start_i = a.row_ptr[row];
            unsigned int end_i = a.row_ptr[row + 1];
            double rank = 0;
            for (unsigned int i = start_i; i < end_i; i++) {
                int dest = a.col_ids[i];
                double weight_i = a.vals[i];
                double sum = 0;
                unsigned int start_j = a.row_ptr[dest];
                unsigned int end_j = a.row_ptr[dest + 1];
                
                for (unsigned int j = start_j; j < end_j; j++) {
                    double weight_j = a.vals[j];
                    unsigned int vert = a.col_ids[j];
                    unsigned int count_inc_edges = a.row_ptr[vert + 1] - a.row_ptr[vert];
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

        PAPI_stop(eventset, info);

        std::cout << "2PAPI_L1_DCM: " << info[0] << std::endl;
        std::cout << "2PAPI_L2_DCM: " << info[1] << std::endl;
        std::cout << "2PAPI native: " << info[2] << std::endl;
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
