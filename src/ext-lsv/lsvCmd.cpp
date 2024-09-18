#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <vector>
#include <algorithm>
#include <unordered_map>

typedef std::vector<int> Cut;

// Custom function to merge two sorted vectors
Cut mergeSortedCuts(const Cut& cut1, const Cut& cut2) {
    Cut result;
    size_t i = 0, j = 0;

    while (i < cut1.size() && j < cut2.size()) {
        if (cut1[i] < cut2[j]) {
            result.push_back(cut1[i++]);
        } else if (cut2[j] < cut1[i]) {
            result.push_back(cut2[j++]);
        } else {
            result.push_back(cut1[i++]);
            j++;
        }
    }

    // Add remaining elements from cut1, if any
    while (i < cut1.size()) {
        result.push_back(cut1[i++]);
    }

    // Add remaining elements from cut2, if any
    while (j < cut2.size()) {
        result.push_back(cut2[j++]);
    }

    return result;
}

static int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandPrintCuts(Abc_Frame_t* pAbc, int argc, char** argv);

void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_nodes", Lsv_CommandPrintNodes, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_printcut", Lsv_CommandPrintCuts, 0);
}

void destroy(Abc_Frame_t* pAbc) {}

Abc_FrameInitializer_t frame_initializer = {init, destroy};

struct PackageRegistrationManager {
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer); }
} lsvPackageRegistrationManager;

void Lsv_NtkPrintNodes(Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;
  Abc_NtkForEachNode(pNtk, pObj, i) {
    printf("Object Id = %d, name = %s\n", Abc_ObjId(pObj), Abc_ObjName(pObj));
    Abc_Obj_t* pFanin;
    int j;
    Abc_ObjForEachFanin(pObj, pFanin, j) {
      printf("  Fanin-%d: Id = %d, name = %s\n", j, Abc_ObjId(pFanin),
             Abc_ObjName(pFanin));
    }
    if (Abc_NtkHasSop(pNtk)) {
      printf("The SOP of this node:\n%s", (char*)pObj->pData);
    }
  }
}

int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }
  Lsv_NtkPrintNodes(pNtk);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_print_nodes [-h]\n");
  Abc_Print(-2, "\t        prints the nodes in the network\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}

void Lsv_NtkPrintKFeasibleCuts(Abc_Ntk_t* pNtk, int k) {
  std::unordered_map<int, std::vector<Cut>> nodeCuts;

  Abc_Obj_t* pObj;
  int i;

  // Enumerate cuts for each node in topological order
  Abc_NtkForEachNode(pNtk, pObj, i) {
    int nodeId = Abc_ObjId(pObj);
    std::vector<Cut>& cuts = nodeCuts[nodeId];

    // Trivial cut (the node itself)
    cuts.push_back({nodeId});

    // Get the two fanins
    Abc_Obj_t* pFanin0 = Abc_ObjFanin0(pObj);
    Abc_Obj_t* pFanin1 = Abc_ObjFanin1(pObj);

    int faninId0 = Abc_ObjId(pFanin0);
    int faninId1 = Abc_ObjId(pFanin1);

    std::vector<Cut>& faninCuts0 = nodeCuts[faninId0];
    std::vector<Cut>& faninCuts1 = nodeCuts[faninId1];

    // Cross-product of cuts from the two fanins
    for (const Cut& cut0 : faninCuts0) {
      for (const Cut& cut1 : faninCuts1) {
        Cut mergedCut = mergeSortedCuts(cut0, cut1);
        if (mergedCut.size() <= k) {
          cuts.push_back(mergedCut);
        }
      }
    }

    // Remove duplicate cuts
    std::sort(cuts.begin(), cuts.end());
    cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());

    // Sort cuts by size and remove cuts larger than k
    std::sort(cuts.begin(), cuts.end(), 
      [](const Cut& a, const Cut& b) { 
        return a.size() < b.size() || (a.size() == b.size() && a < b); 
      }
    );
    cuts.erase(std::remove_if(cuts.begin(), cuts.end(),
      [k](const Cut& cut) { return cut.size() > k; }),
      cuts.end());

    // Print cuts for this node
    for (const Cut& cut : cuts) {
      printf("%d:", nodeId);
      for (int cutNode : cut) {
        printf(" %d", cutNode);
      }
      printf("\n");
    }
  }
}

// Add this function after Lsv_CommandPrintNodes
int Lsv_CommandPrintCuts(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int c, k = 0;

  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }

  if (argc != globalUtilOptind + 1) {
    goto usage;
  }

  k = atoi(argv[globalUtilOptind]);
  if (k < 3 || k > 6) {
    Abc_Print(-1, "Error: k must be between 3 and 6.\n");
    goto usage;
  }

  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }

  // Ensure the network is an AIG
  if (!Abc_NtkIsStrash(pNtk)) {
    Abc_Print(-1, "The network is not in AIG format. Please run 'strash' first.\n");
    return 1;
  }

  Lsv_NtkPrintKFeasibleCuts(pNtk, k);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_printcut <k>\n");
  Abc_Print(-2, "\t        prints k-feasible cuts for each node in the network\n");
  Abc_Print(-2, "\t<k>   : the maximum cut size (3 <= k <= 6)\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}
