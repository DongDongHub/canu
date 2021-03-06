
/******************************************************************************
 *
 *  This file is part of canu, a software program that assembles whole-genome
 *  sequencing reads into contigs.
 *
 *  This software is based on:
 *    'Celera Assembler' (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' (http://kmer.sourceforge.net)
 *  both originally distributed by Applera Corporation under the GNU General
 *  Public License, version 2.
 *
 *  Canu branched from Celera Assembler at its revision 4587.
 *  Canu branched from the kmer project at its revision 1994.
 *
 *  Modifications by:
 *
 *    Brian P. Walenz from 2015-MAR-27 to 2015-JUN-25
 *      are Copyright 2015 Battelle National Biodefense Institute, and
 *      are subject to the BSD 3-Clause License
 *
 *    Sergey Koren beginning on 2016-FEB-24
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *    Brian P. Walenz beginning on 2016-AUG-09
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *  File 'README.licenses' in the root directory of this distribution contains
 *  full conditions and disclaimers for each license.
 */

#include "AS_global.H"
#include "ovStore.H"
#include "splitToWords.H"

#include <vector>

using namespace std;


int
main(int argc, char **argv) {
  char           *outName     = NULL;
  char           *seqName     = NULL;

  vector<char *>  files;


  int32     arg = 1;
  int32     err = 0;
  while (arg < argc) {
    if        (strcmp(argv[arg], "-o") == 0) {
      outName = argv[++arg];

    } else if (strcmp(argv[arg], "-S") == 0) {
      seqName = argv[++arg];

    } else if (AS_UTL_fileExists(argv[arg])) {
      files.push_back(argv[arg]);

    } else {
      fprintf(stderr, "ERROR:  invalid arg '%s'\n", argv[arg]);
      err++;
    }

    arg++;
  }

  if ((err) || (seqName == NULL) || (outName == NULL) || (files.size() == 0)) {
    fprintf(stderr, "usage: %s -S seqStore -o output.ovb input.mhap[.gz]\n", argv[0]);
    fprintf(stderr, "  Converts mhap native output to ovb\n");

    if (seqName == NULL)
      fprintf(stderr, "ERROR:  no seqStore (-S) supplied\n");
    if (files.size() == 0)
      fprintf(stderr, "ERROR:  no overlap files supplied\n");

    exit(1);
  }

  char       *ovStr = new char [1024];

  sqStore    *seqStore = sqStore::sqStore_open(seqName);
  ovOverlap   ov(seqStore);
  ovFile     *of = new ovFile(seqStore, outName, ovFileFullWrite);


  for (uint32 ff=0; ff<files.size(); ff++) {
    compressedFileReader  *in = new compressedFileReader(files[ff]);

    //  $1    $2   $3       $4  $5  $6  $7   $8   $9  $10 $11  $12
    //  0     1    2        3   4   5   6    7    8   9   10   11
    //  26887 4509 87.05933 301 0   479 2305 4328 1   34  1852 3637
    //  aiid  biid qual     ?   ori bgn end  len  ori bgn end  len

    while (fgets(ovStr, 1024, in->file()) != NULL) {
      splitToWords  W(ovStr);

      char   *aid = W[0];
      char   *bid = W[1];

      if ((aid[0] == 'r') && (aid[1] == 'e') && (aid[2] == 'a') && (aid[3] == 'd'))
        aid += 4;

      if ((bid[0] == 'r') && (bid[1] == 'e') && (bid[2] == 'a') && (bid[3] == 'd'))
        bid += 4;

      ov.a_iid = strtouint32(aid);      //  First ID is the query
      ov.b_iid = strtouint32(bid);      //  Second ID is the hash table

      if (ov.a_iid == ov.b_iid)
        continue;

      assert(W[4][0] == '0');   //  first read is always forward

      assert(W(5)  <  W(6));    //  first read bgn < end
      assert(W(6)  <= W(7));    //  first read end <= len

      assert(W(9)  <  W(10));   //  second read bgn < end
      assert(W(10) <= W(11));   //  second read end <= len

      ov.dat.ovl.forUTG = true;
      ov.dat.ovl.forOBT = true;
      ov.dat.ovl.forDUP = true;

      ov.dat.ovl.ahg5 = W(5);
      ov.dat.ovl.ahg3 = W(7) - W(6);

      if (W[8][0] == '0') {
        ov.dat.ovl.bhg5 = W(9);
        ov.dat.ovl.bhg3 = W(11) - W(10);
        ov.flipped(false);
      } else {
        ov.dat.ovl.bhg5 = W(11) - W(10);
        ov.dat.ovl.bhg3 = W(9);
        ov.flipped(true);
      }

      ov.erate(atof(W[2]));

      //  Check the overlap - the hangs must be less than the read length.

      uint32  alen = seqStore->sqStore_getRead( ov.a_iid )->sqRead_sequenceLength();
      uint32  blen = seqStore->sqStore_getRead( ov.b_iid )->sqRead_sequenceLength();

      if ((alen != W(7)) ||
          (blen != W(11)))
        fprintf(stderr, "%s\nINVALID LENGTHS read " F_U32 " (len %d) and read " F_U32 " (len %d) lengths " F_S64 " and " F_S64 "\n",
                ovStr,
                ov.a_iid, alen,
                ov.b_iid, blen,
                W(7), W(11)), exit(1);

      if ((alen < ov.dat.ovl.ahg5 + ov.dat.ovl.ahg3) ||
          (blen < ov.dat.ovl.bhg5 + ov.dat.ovl.bhg3))
        fprintf(stderr, "%s\nINVALID OVERLAP read " F_U32 " (len %d) and read " F_U32 " (len %d) hangs " F_U64 "/" F_U64 " and " F_U64 "/" F_U64 "%s\n",
                ovStr,
                ov.a_iid, alen,
                ov.b_iid, blen,
                ov.dat.ovl.ahg5, ov.dat.ovl.ahg3,
                ov.dat.ovl.bhg5, ov.dat.ovl.bhg3,
                (ov.dat.ovl.flipped) ? " flipped" : ""), exit(1);

      //  Overlap looks good, write it!

      of->writeOverlap(&ov);
    }

    delete in;

    arg++;
  }

  delete    of;
  delete [] ovStr;

  seqStore->sqStore_close();

  exit(0);
}
