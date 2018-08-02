#include "ValidationParser.h"

// Global variables
bool hasConfig=true;
TTree *tree;
map<string,string> configParams;
string plotdir;
int MAINWALL_WIDTH = 20;
int MAINWALL_HEIGHT = 13;
int XWALL_DEPTH = 4;
int XWALL_HEIGHT = 16;
int VETO_DEPTH = 2;
int VETO_WIDTH = 16;

/**
 *  main function
 * Arguments are <root file> <config file (optional)>
 */
int main(int argc, char **argv)
{
  gStyle->SetOptStat(0);
  if (argc < 2)
  {
    //cout<<"Usage: "<<argv[0]<<" <root file> <config file (optional)>"<<endl;
    cout<<"Usage: "<<argv[0]<<" -i <data ROOT file> -r <reference ROOT file (optional)> -c <config file (optional)>"<<endl;
    return -1;
  }
  // This bit is kept for compatibility with old version that would take just a root file name and a config file name
  string dataFileInput="";
  string referenceFileInput="";
  string configFileInput="";
  if (argc == 2)
  {
    dataFileInput = argv[1];
  }
  else if (argc == 3 && argv[1][0]!= '-')
  {
    dataFileInput = argv[1];
    configFileInput = (argv[2]);
  }
  else
  {
    int flag=0;
    while ((flag = getopt (argc, argv, "i:r:c:")) != -1)
    {
      switch (flag)
      {
        case 'i':
          dataFileInput = optarg;
          break;
        case 'r':
          referenceFileInput = optarg;
          break;
        case 'c':
          configFileInput = optarg;
          break;
        case '?':
          if (optopt == 'i' || optopt == 'r' || optopt == 'c')
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          else if (isprint (optopt))
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          else
            fprintf (stderr,
                     "Unknown option character `\\x%x'.\n",
                     optopt);
          return 1;
        default:
          abort ();
      }
    }
  }

  if (dataFileInput.length()<=0)
  {
    cout<<"ERROR: Data file name is needed."<<endl;
    cout<<"Usage: "<<argv[0]<<" -i <data ROOT file> -r <reference ROOT file (optional)> -c <config file (optional)>"<<endl;
    return -1;
  }
  ParseRootFile(dataFileInput,configFileInput,referenceFileInput);
  return 0;
}


/**
 *  Main work function - parses a ROOT file and plots the variables in the branches
 *  rootFileName: path to the ROOT file with SuperNEMO validation data
 *  configFileName: optional to specify how to plot certain variables
 */
void ParseRootFile(string rootFileName, string configFileName, string refFileName)
{

  // Check the input root file can be opened and contains a tree with the right name
  cout<<"Processing "<<rootFileName<<endl;
  TFile *rootFile;

  rootFile = new TFile(rootFileName.c_str());
  if (rootFile->IsZombie())
  {
    cout<<"Error: file "<<rootFileName<<" not found"<<endl;
    return ;
  }
  tree = (TTree*) rootFile->Get(treeName.c_str()); // Name is in the .h file for now
  // Check if it found the tree
  if (tree==0)
    {
      cout<<"Error: no data in a tree named "<<treeName<<endl;
      return;
    }

  // Check if we have a config file
  ifstream configFile (configFileName.c_str());
  if (configFileName.length()==0 ) // No config file given
  {
    cout<<"No config file provided - using default settings"<<endl;
    hasConfig=false;
  }
  else if  (!configFile) // file name given but file not found
  {
    cout<<"WARNING: Config file "<<configFileName<<" not found - using default settings"<<endl;
    hasConfig=false;
  }
  else
  {
    cout<<"Using config file "<<configFileName<<endl;
    configParams=LoadConfig(configFile);
  }
  
  // Make a directory to put the plots in
  string rootFileNameNoPath=rootFileName;
  try{
    rootFileNameNoPath=rootFileName.substr(rootFileName.find_last_of("/")+1);
  }
  catch (exception e)
  {
    rootFileNameNoPath=rootFileName;
  }
  plotdir = "plots"+rootFileNameNoPath;
  boost::filesystem::path dir(plotdir.c_str());
  if(boost::filesystem::create_directory(dir))
  {
    cout<< "Directory Created: "<<plotdir<<std::endl;
  }
  // In the plots directory, make an output ROOT file for the histograms
  TFile *outputFile=new TFile((plotdir+"/ValidationHistograms.root").c_str(),"RECREATE");
  outputFile->cd();
  
  // Get a list of all the branches in the tree
  TObjArray* branches = tree->GetListOfBranches();
  TIter next(branches);
  TBranch *branch;
  
  // Loop the branches and decide how to treat them based on the first character of the name
  while( (branch=(TBranch *)next() )){
    string branchName=branch->GetName();
    PlotVariable(branchName);
  }
  
  if (configFile.is_open()) configFile.close();
  outputFile->Close();
  return;
}

/**
 *  Decides what plot to make for a branch
 *  depending on the prefix
 */
bool PlotVariable(string branchName)
{
  string config="";
  bool configFound=false;
  // If we have a config file, check if this var is in it
  if (hasConfig)
  {
    config = configParams[branchName];
    if (config.length() > 0)
    {
      configFound=true;
    }
  }
  switch (branchName[0])
  {
    case 'h':
    {
      Plot1DHistogram(branchName);
      break;
    }
    case 't':
    {
      PlotTrackerMap(branchName);
      break;
    }
    case 'c':
    {
      PlotCaloMap(branchName);
      break;
    }
    default:
    {
      //cout<< "Unknown variable type "<<branchName<<": treat as histogram"<<endl;
    }
  }

  return true;
}


/**
 *  Loads the config file (which should be short) into a memory
 *  map where the key is the branch name (first item in the CSV line)
 *  value is the rest of the line
 */
map<string,string> LoadConfig(ifstream& configFile)
{
  map<string,string> configLookup;
  string thisLine;
  
  while (configFile)
  {
    getline(configFile, thisLine);
    string key=GetBitBeforeComma(thisLine);
    configLookup[key]=thisLine; // the remainder of the line
  }
  return configLookup;
}

/**
 *  Make a basic histogram of a variable. The number of bins etc will come from
 *  the config file if there is one, if not we will guess
 */
void Plot1DHistogram(string branchName)
{
  int notSetVal=-9999;
  string config="";
  config=configParams[branchName]; // get the config loaded from the file if there is one
  int nbins=100;
  int lowLimit=0;
  int highLimit=notSetVal;
  string title="";
  if (config.length()>0)
  {
    // title, nbins, low limit, high limit separated by commas
    title=GetBitBeforeComma(config); // config now has this bit chopped off ready for the next parsing stage

    // Number of bins
    try
    {
      string nbinString=GetBitBeforeComma(config);
      std::string::size_type sz;   // alias of size_t
      nbins = std::stoi (nbinString,&sz); // hopefully the next chunk is turnable into an integer
    }
    catch (exception &e)
    {
      nbins=0;
    }
    
    // Low bin limit
    try
    {
      string lowString=GetBitBeforeComma(config);
      lowLimit = std::stod (lowString); // hopefully the next chunk is turnable into an double
    }
    catch (exception &e)
    {
      lowLimit=0;
    }
    
    // High bin limit
    try
    {
      string highString=GetBitBeforeComma(config);
      highLimit = std::stod (highString); // hopefully the next chunk is turnable into an double
    }
    catch (exception &e)
    {
      highLimit=notSetVal;
    }
  }
  

  if (title.length()==0)
  {
    title = BranchNameToEnglish(branchName);
  }
  TCanvas *c = new TCanvas (("plot_"+branchName).c_str(),("plot_"+branchName).c_str(),900,600);
  TH1D *h;
  tree->Draw(branchName.c_str());
  if (highLimit == notSetVal)
  {
    // Use the default limits
    TH1D *tmp = (TH1D*) gPad->GetPrimitive("htemp");
    highLimit=tmp->GetXaxis()->GetXmax();
    delete tmp;
    EDataType datatype;
    TClass *ctmp;
    tree->FindBranch(branchName.c_str())->GetExpectedType(ctmp,datatype);
    delete ctmp;
    if (datatype==kBool_t)
    {
      nbins=2;
      highLimit=2;
      lowLimit=0;
    }
    else if (datatype== kInt_t || datatype== kUInt_t)
    {
      
      highLimit +=1;
      if (highLimit <=100) nbins = (int) highLimit;
      else nbins = 100;
    }
    else
    {
      highLimit += highLimit /10.;
      nbins = 100;
    }
  }
  h = new TH1D(("plt_"+branchName).c_str(),title.c_str(),nbins,lowLimit,highLimit);
  h->GetYaxis()->SetTitle("Events");
  h->GetXaxis()->SetTitle(title.c_str());
  h->SetFillColor(kPink-6);
  h->SetFillStyle(1);
  tree->Draw((branchName + ">> plt_"+branchName).c_str());
  h->Write("",TObject::kOverwrite);
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  delete h;
  delete c;
}


/**
 *  Plot a map of the calorimeter walls
 *  We have 6 walls in total : 2 main walls (Italy, France)
 *  2 x walls (tunnel, mountain) and 2 gamma vetos (top, bottom)
 *  This makes two kinds of map: for c_... variables, it just adds 1 for every listed calo
 *  for cm... variables it needs to use two branches - the cm_ one and its corresponding c_ one
 *  and then it uses the calorimeter locations in the c_ variable to calculate the mean for
 *  each calorimeter, based on the paired numbers in the cm_ variable
 */
void PlotCaloMap(string branchName)
{
  
  string config="";
  
  string fullBranchName=branchName;//This is a combo of name and parent for average branches
  
  // is it an average?
  string mapBranch=branchName;
  
  bool isAverage=false;
  if (branchName[1]=='m')
  {
    // In this case, the branch name should be split in two with a . character
    int pos=branchName.find(".");
    if (pos<=1)
    {
      cout<<"Error - could not find map branch for "<<branchName<<": remember to provide a map branch name with a dot"<<endl;
      return;
    }
    mapBranch=branchName.substr(pos+1);
    branchName=branchName.substr(0,pos);
    isAverage=true;
  }

  config=configParams[branchName]; // get the config loaded from the file if there is one
  
  string title="";
  // Load the title from the config file
  if (config.length()>0)
  {
    // title is the only thing for this one
    title=GetBitBeforeComma(config); // config now has this bit chopped off ready for the next parsing stage
  }
  // Set the title to a default if there isn't anything in the config file
  if (title.length()==0)
  {
    title = BranchNameToEnglish(branchName);
  }
  
  // Make 6 2-dimensional histograms for the 6 walls
  TH2D *hItaly = new TH2D(("plt_"+branchName+"_italy").c_str(),"Italy",MAINWALL_WIDTH,-1*MAINWALL_WIDTH,0,MAINWALL_HEIGHT,0,MAINWALL_HEIGHT); // Italian side main wall
  TH2D *hFrance = new TH2D(("plt_"+branchName+"_france").c_str(),"France",MAINWALL_WIDTH,0,MAINWALL_WIDTH,MAINWALL_HEIGHT,0,MAINWALL_HEIGHT); // France side main wall
  TH2D *hTunnel = new TH2D(("plt_"+branchName+"_tunnel").c_str(),"Tunnel", XWALL_DEPTH ,-1 * XWALL_DEPTH/2,XWALL_DEPTH/2,XWALL_HEIGHT,0,XWALL_HEIGHT); // Tunnel side x wall
  TH2D *hMountain = new TH2D(("plt_"+branchName+"_mountain").c_str(),"Mountain", XWALL_DEPTH ,-1 * XWALL_DEPTH/2,XWALL_DEPTH/2,XWALL_HEIGHT,0,XWALL_HEIGHT); // Mountain side x wall
  TH2D *hTop = new TH2D(("plt_"+branchName+"_top").c_str(),"Top", VETO_WIDTH ,0,VETO_WIDTH,VETO_DEPTH,0,VETO_DEPTH); //Top gamma veto
  TH2D *hBottom = new TH2D(("plt_"+branchName+"_bottom").c_str(),"Bottom", VETO_WIDTH ,0,VETO_WIDTH,VETO_DEPTH,0,VETO_DEPTH); // Bottom gamma veto

  // Make histos for avereages
  TH2D *mItaly = new TH2D(("ave_"+branchName+"_italy").c_str(),"Italy",MAINWALL_WIDTH,-1*MAINWALL_WIDTH,0,MAINWALL_HEIGHT,0,MAINWALL_HEIGHT); // Italian side main wall
  TH2D *mFrance = new TH2D(("ave_"+branchName+"_france").c_str(),"France",MAINWALL_WIDTH,0,MAINWALL_WIDTH,MAINWALL_HEIGHT,0,MAINWALL_HEIGHT); // France side main wall
  TH2D *mTunnel = new TH2D(("ave_"+branchName+"_tunnel").c_str(),"Tunnel", XWALL_DEPTH ,-1 * XWALL_DEPTH/2,XWALL_DEPTH/2,XWALL_HEIGHT,0,XWALL_HEIGHT); // Tunnel side x wall
  TH2D *mMountain = new TH2D(("ave_"+branchName+"_mountain").c_str(),"Mountain", XWALL_DEPTH ,-1 * XWALL_DEPTH/2,XWALL_DEPTH/2,XWALL_HEIGHT,0,XWALL_HEIGHT); // Mountain side x wall
  TH2D *mTop = new TH2D(("ave_"+branchName+"_top").c_str(),"Top", VETO_WIDTH ,0,VETO_WIDTH,VETO_DEPTH,0,VETO_DEPTH); //Top gamma veto
  TH2D *mBottom = new TH2D(("ave_"+branchName+"_bottom").c_str(),"Bottom", VETO_WIDTH ,0,VETO_WIDTH,VETO_DEPTH,0,VETO_DEPTH); // Bottom gamma veto

  // Loop the event tree and decode the position
  
  // Count the number of entries in the tree
  int nEntries = tree -> GetEntries();
  // Set up a vector of strings to receive the list of calorimeter IDs
  // There is one entry in the vector for each calorimeter hit in the event
  // And it will have a format something like [1302:0.1.0.10.*]
  std::vector<string> *caloHits = 0;
  
  TTree *thisTree=tree->CopyTree("");

  thisTree->SetBranchAddress(mapBranch.c_str(), &caloHits);
  
  std::vector<double> *toAverage =0;
  if (isAverage)
  {
    thisTree->SetBranchAddress(fullBranchName.c_str(), &toAverage);
  }

  // Loop through the tree
  for( int iEntry = 0; iEntry < nEntries; iEntry++ )
  {
    thisTree->GetEntry(iEntry);
    // Populate these with which histogram we will fill and what cell
    int xValue=0;
    int yValue=0;
    TH2D *whichHistogram=0;
    TH2D *whichAverage=0; // this is a tad messy
    // This should always work, but there is next to no catching of badly formatted
    // geom ID strings. Are they a possibility?
    if (caloHits->size()>0)
    {
      for (int i=0;i<caloHits->size();i++)
      {
        string thisHit=caloHits->at(i);
        //cout<<thisHit<<endl;
        if (thisHit.length()>=9)
        {
          bool isFrance=(thisHit.substr(8,1)=="1");
          //Now to decode it
          string wallType = thisHit.substr(1,4);
          
          if (wallType=="1302") // Main walls
          {
            
            if (isFrance) whichHistogram = hFrance; else whichHistogram = hItaly;
            string useThisToParse = thisHit;
            
            // Hacky way to get the bit between the 2nd and 3rd "." characters for x
            int pos=useThisToParse.find('.');
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find('.');
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find('.');
            std::string::size_type sz;   // alias of size_t
            xValue = std::stoi (useThisToParse.substr(0,pos),&sz);

            // and the bit before the next . characters for y
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find_first_of('.');
            yValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            
            // The numbering is from mountain to tunnel
            // But we draw the Italian side as we see it, with the mountain on the left
            // So let's flip it around
            if (!isFrance)xValue = -1 * (xValue + 1);
            //cout<<iEntry<<" : " <<thisHit<<" : " <<(isFrance?"Fr.":"It")<<" - "<<xValue<<":"<<yValue<<endl;
          }
          else if (wallType == "1232") //x walls
          {
            bool isTunnel=(thisHit.substr(10,1)=="1");
            if (isTunnel) whichHistogram = hTunnel; else whichHistogram = hMountain;
            // Hacky way to get the bit between the 3rd and 4th "." characters for x
            string useThisToParse = thisHit;
            int pos=0;
            for (int j=0;j<3;j++)
            {
              int pos=useThisToParse.find('.');
              useThisToParse=useThisToParse.substr(pos+1);
            }
            pos=useThisToParse.find('.');
            std::string::size_type sz;   // alias of size_t
            xValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            
            // and the bit before the next . characters for y
            useThisToParse=useThisToParse.substr(pos+1);
            pos=useThisToParse.find_first_of('.');
            yValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            if (!isFrance)xValue = -1 * (xValue + 1); // Italy is on the left so reverse these to draw them
            
            if (isTunnel) // Switch it so France is on the left for the tunnel side
            {
              xValue = -1 * (xValue + 1);
            }
            
         //cout<<iEntry<<" : " <<thisHit<<" : " <<(isFrance?"Fr.":"It")<<" - "<<(isTunnel?"tunnel":"mountain")<<" - "<<xValue<<":"<<yValue<<endl;

          }
          else if (wallType == "1252") // veto walls
          {
            bool isTop=(thisHit.substr(10,1)=="1");
            if (isTop) whichHistogram = hTop; else whichHistogram = hBottom;
            string useThisToParse = thisHit;
            int pos=useThisToParse.find('.');
            for (int j=0;j<4;j++)
            {
              useThisToParse=useThisToParse.substr(pos+1);
              pos=useThisToParse.find('.');
            }
            
            std::string::size_type sz;   // alias of size_t
            yValue=((isFrance^isTop)?1:0); // We flip this so that French side is inwards on the print
            xValue = std::stoi (useThisToParse.substr(0,pos),&sz);
            //cout<<iEntry<<" : " <<(isFrance?"Fr.":"It")<<" "<<(isTop?"top ":"bottom ")<<xValue<<endl;
          }
          else
          {
            cout<<"WARNING -- Calo hit found with unknown wall type "<<wallType<<endl;
            continue; // We can't plot it if we don't know where to plot it
          }
          
          // Now we know which histogram and the coordinates so write it
          whichHistogram->Fill(xValue,yValue);
          // This could be improved! Maybe a map of an enum to the sets of hists
          if (whichHistogram==hFrance) whichAverage =mFrance;
          if (whichHistogram==hItaly) whichAverage =mItaly;
          if (whichHistogram==hTunnel) whichAverage =mTunnel;
          if (whichHistogram==hMountain) whichAverage =mMountain;
          if (whichHistogram==hTop) whichAverage =mTop;
          if (whichHistogram==hBottom) whichAverage =mBottom;
          if (isAverage)
          {
            whichAverage->Fill(xValue,yValue,toAverage->at(i));
          }
        }// end parsable string
      } // end for each hit
    } // End if there are calo hits
  }
  if (isAverage)
  {
    mFrance->Divide(hFrance); mFrance->Write("",TObject::kOverwrite);
    mItaly->Divide(hItaly); mItaly->Write("",TObject::kOverwrite);
    mTunnel->Divide(hTunnel); mTunnel->Write("",TObject::kOverwrite);
    mMountain->Divide(hMountain); mMountain->Write("",TObject::kOverwrite);
    mTop->Divide(hTop); mTop->Write("",TObject::kOverwrite);
    mBottom->Divide(hBottom); mBottom->Write("",TObject::kOverwrite);
    
    // Print them all to a png file
    PrintCaloPlots(branchName,title,mItaly,mFrance,mTunnel,mMountain,mTop,mBottom);
  }
  else{
  // Write the histograms to a file
  hFrance->Write("",TObject::kOverwrite);
  hItaly->Write("",TObject::kOverwrite);
  hTunnel->Write("",TObject::kOverwrite);
  hMountain->Write("",TObject::kOverwrite);
  hTop->Write("",TObject::kOverwrite);
  hBottom->Write("",TObject::kOverwrite);
  
  // Print them all to a png file
  PrintCaloPlots(branchName,title,hItaly,hFrance,hTunnel,hMountain,hTop,hBottom);

  }

}

/**
 *  Plot a map of the tracker cells
 */
void PlotTrackerMap(string branchName)
{
  //cout<<endl; // THIS LINE STOPS IT CRASHING BUT WHY?!?!
  int maxlayers=9;
  int maxrows=113;
  
  string config="";
  config=configParams[branchName]; // get the config loaded from the file if there is one
  
  string title="";
  // Load the title from the config file
  if (config.length()>0)
  {
    // title is the only thing for this one
    title=GetBitBeforeComma(config); // config now has this bit chopped off ready for the next parsing stage
  }
  
  string fullBranchName=branchName;//This is a combo of name and parent for average branches
  // is it an average?
  string mapBranch=branchName;
  bool isAverage=false;
  if (branchName[1]=='m')
  {
    // In this case, the branch name should be split in two with a . character
    int pos=branchName.find(".");
    if (pos<=1)
    {
      cout<<"Error - could not find map branch for "<<branchName<<": remember to provide a map branch name with a dot"<<endl;
      return;
    }
    mapBranch=branchName.substr(pos+1);
    branchName=branchName.substr(0,pos);
    isAverage=true;
  }
  
  // Set the title to a default if there isn't anything in the config file
  if (title.length()==0)
  {
    title = BranchNameToEnglish(branchName);
  }


  // Make the plot
  TCanvas *c = new TCanvas (("plot_"+branchName).c_str(),("plot_"+branchName).c_str(),600,1200);
  TH2D *h = new TH2D(("plt_"+branchName).c_str(),title.c_str(),maxlayers*2,maxlayers*-1,maxlayers,maxrows,0,maxrows); // Map of the tracker
  h->Sumw2(); // Important to get errors right
  TH2D *hAve = new TH2D(("ave_"+branchName).c_str(),title.c_str(),maxlayers*2,maxlayers*-1,maxlayers,maxrows,0,maxrows); // Map of the tracker
  hAve->Sumw2(); // Important to get errors right
  h->GetYaxis()->SetTitle("Row");
  h->GetXaxis()->SetTitle("Layer");

  // This decodes the encoded tracker map to extract the x and y positions

  // Unfortunately it is not so easy to make the averages plot so we need to loop the tuple
  std::vector<int> *trackerHits = 0;
  
  std::vector<double> *toAverageTrk = 0;
  TTree *thisTree=tree->CopyTree("");

  thisTree->SetBranchAddress(mapBranch.c_str(), &trackerHits);


  if (isAverage)
  {
    thisTree->SetBranchAddress(fullBranchName.c_str(), &toAverageTrk);
  }
  
  //return;

  // Now we can fill the two plots
  // Loop through the tree
  
  int nEntries = tree -> GetEntries();
  for( int iEntry = 0; iEntry < nEntries; iEntry++ )
  {
    thisTree->GetEntry(iEntry);
    // Populate these with which histogram we will fill and what cell
    int xValue=0;
    int yValue=0;
    if (trackerHits->size()>0)
    {
      for (int i=0;i<trackerHits->size();i++)
      {
        yValue=TMath::Abs(trackerHits->at(i)/100);
        xValue=trackerHits->at(i)%100;
        if (isAverage)
          hAve->Fill(xValue,yValue,toAverageTrk->at(i));
        h->Fill(xValue,yValue);
      }
    }
  }

  if (isAverage)
  {
    hAve->Divide(h);
    h=hAve; // overwrite the temp plot with the one we actually want to save
  }
  
  h->Draw("COLZ");
  
  // Annotate to make it clear what the detector layout is
  TLine *foil=new TLine(0,0,0,113);
  foil->SetLineColor(kGray);
  foil->SetLineWidth(5);
  foil->Draw("SAME");
  
  // Decorate the print
  WriteLabel(.2,.5,"Italy");
  WriteLabel(.7,.5,"France");
  WriteLabel(0.4,.8,"Tunnel");
  WriteLabel(.4,.15,"Mountain");
  h->Write("",TObject::kOverwrite);
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  delete h;
  delete c;

}

// Just a quick routine to write text at a (x,y) coordinate
void WriteLabel(double x, double y, string text, double size)
{
  TText *txt = new TText(x,y,text.c_str());
  txt->SetTextSize(size);
  txt->SetNDC();
  txt->Draw();

}
/**
 *  Return the part of the string that is before the first comma (trimmed of white space)
 *  Modify the input string to be whatever is AFTER the first comma
 *  If there is no comma, return the whole (trimmed) string and modify the input to zero-length string
 */
string GetBitBeforeComma(string& input)
{
  string output;
  int pos=input.find_first_of(',');
  if (pos <=0)
  {
    boost::trim(input);
    output=input;
    input="";
  }
  else
  {
    output=input.substr(0,pos);
    boost::trim(output);
    input=input.substr(pos+1);
  }
  return output;
}

string BranchNameToEnglish(string branchname)
{
  int pos = branchname.find_first_of("_");
  int initpos=pos;
  while (pos >=0)
  {
    branchname.replace(pos,1," ");
    pos = branchname.find_first_of("_");
  }
  string output = branchname.substr(initpos+1,branchname.length());
  output[0]=toupper(output[0]);
  return output;
}

// Arrange all the bits of calorimeter on a canvas
void PrintCaloPlots(string branchName, string title, TH2* hItaly,TH2* hFrance,TH2* hTunnel,TH2* hMountain,TH2* hTop,TH2* hBottom)
{
  
  TCanvas *c = new TCanvas ("caloplots","caloplots",2000,1000);
  std::vector <TH2*> histos;
  histos.push_back(hItaly);
  histos.push_back(hFrance);
  histos.push_back(hTunnel);
  histos.push_back(hMountain);
  histos.push_back(hTop);
  histos.push_back(hBottom);
  
  //First, 20x13
  TPad *pItaly = new TPad("p_italy",
                          "",0.6,0.2,1,0.8,0);
  // Third 20x13
  TPad *pFrance = new TPad("p_france",
                           "",0.1,0.2,0.5,0.8,0);
  // Second, 4x16
  TPad *pMountain = new TPad("p_mountain",
                             "",0.02,0.2,0.12,0.8,0);
  // Fourth 4x16
  TPad *pTunnel = new TPad("p_tunnel",
                           "",0.5,0.2,.6,0.8,0);
  // 16x2
  TPad *pTop = new TPad("p_top",
                        "",0.1,0.8,0.5,.98,0);
  //16 x2
  TPad *pBottom = new TPad("p_bottom",
                           "",0.1,0.02,0.5,0.2,0);
  TPad *pTitle = new TPad("p_title",
                          "",0.6,0.8,0.95,1,0);
  pTitle->Draw();
  
  std::vector <TPad*> pads;
  pads.push_back(pItaly);
  pads.push_back(pTunnel);
  pads.push_back(pFrance);
  pads.push_back(pMountain);
  pads.push_back(pTop);
  pads.push_back(pBottom);
  
  
  hBottom->GetYaxis()->SetBinLabel(2,"France");
  hBottom->GetYaxis()->SetBinLabel(1,"Italy");
  hTop->GetYaxis()->SetBinLabel(1,"France");
  hTop->GetYaxis()->SetBinLabel(2,"Italy");
  
  gStyle->SetOptTitle(0);
  
  // Set them all to the same scale; first work out what it should be
  double max=-9999;
  for (int i=0;i<histos.size();i++)
  {
    max=TMath::Max(max,(double)histos.at(i)->GetMaximum());
    // While we are here, let's draw the pads
    pads.at(i)->Draw();pads.at(i)->SetGrid();
  }
  
  gStyle->SetGridStyle(3);
  gStyle->SetGridColor(kGray);
  for (int i=0;i<histos.size();i++)
  {
    histos.at(i)->GetZaxis()->SetRangeUser(0,max);
    histos.at(i)->GetYaxis()->SetNdivisions(histos.at(i)->GetNbinsY());
    histos.at(i)->GetXaxis()->SetNdivisions(histos.at(i)->GetNbinsX());
    histos.at(i)->GetXaxis()->CenterLabels();
    histos.at(i)->GetYaxis()->CenterLabels();
    
  }
  
  // Italian Main wall
  pItaly->cd();
  for (int i=1;i<=hItaly->GetNbinsX();i++)
  {
    hItaly->GetXaxis()->SetBinLabel(i,(to_string(MAINWALL_WIDTH-i)).c_str());
  }
  
  hItaly->GetXaxis()->SetLabelSize(0.06);
  hItaly->Draw("COLZ"); // Only have the scale over on the right
  WriteLabel(.45,.95,"Italy");
  
  // French main wall
  pFrance->cd();
  hFrance->Draw("COL");
  WriteLabel(.4,.95,"France");
  
  // Mountain x wall
  pMountain->cd();
  hMountain->GetYaxis()->SetLabelSize(0.1);
  hMountain->GetYaxis()->SetLabelOffset(0.01);
  hMountain->GetXaxis()->SetBinLabel(1,"It.");
  hMountain->GetXaxis()->SetBinLabel(2,"");
  hMountain->GetXaxis()->SetBinLabel(3,"");
  hMountain->GetXaxis()->SetBinLabel(4,"Fr.");
  hMountain->GetXaxis()->SetLabelSize(0.2);
  
  hMountain->Draw("COL");
  WriteLabel(.2,.95,"Mountain",0.15);
  // Draw on the source foil
  TLine *foil=new TLine(0,0,0,16);
  foil->SetLineColor(kGray+3);
  foil->SetLineWidth(5);
  foil->Draw("SAME");
  
  // Tunnel x wall
  pTunnel->cd();
  hTunnel->GetYaxis()->SetLabelSize(0.1);
  hTunnel->GetYaxis()->SetLabelOffset(0.01);
  hTunnel->GetXaxis()->SetBinLabel(1,"Fr.");
  hTunnel->GetXaxis()->SetBinLabel(2,"");
  hTunnel->GetXaxis()->SetBinLabel(3,"");
  hTunnel->GetXaxis()->SetBinLabel(4,"It.");
  hTunnel->GetXaxis()->SetLabelSize(0.2);
  hTunnel->Draw("COL");
  WriteLabel(.25,.95,"Tunnel",0.15);
  foil->Draw("SAME");
  
  // Top veto wall
  pTop->cd();
  hTop->Draw("COL");
  hTop->GetXaxis()->SetLabelSize(0.1);
  hTop->GetYaxis()->SetLabelSize(0.15);
  TLine *foilveto=new TLine(0,1,16,1);
  foilveto->SetLineColor(kGray+3);
  foilveto->SetLineWidth(5);
  foilveto->Draw("SAME");
  WriteLabel(.42,.2,"Top",0.2);
  
  
  // Bottom veto wall
  pBottom->cd();
  hBottom->GetXaxis()->SetLabelSize(0.1);
  hBottom->GetYaxis()->SetLabelSize(0.15);
  hBottom->Draw("COL");
  foilveto->Draw("SAME");
  WriteLabel(.42,.6,"Bottom",0.2);
  
  pTitle->cd();
  WriteLabel(.1,.5,title,0.2);
  
  c->SaveAs((plotdir+"/"+branchName+".png").c_str());
  delete c;
  return;
}
