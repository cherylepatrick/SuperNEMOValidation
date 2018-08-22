# SuperNEMOValidation

Last updated July 20, 2018 by Cheryl Patrick

## Purpose
This tool takes a ROOT ntuple file in which branches have a specific naming convention and format, and will produce diagnostic plots and statistics of the data in the file. The plots are automatically generated if the ntuple has been properly generated (with the correct naming and format). It can also compare to a reference.

The tool is currently able to:
- Make one-dimensional histograms of a variable (e.g. total energy in an event, number of tracks in an event)
- Make maps of the tracker showing counts of some variable per tracker cell (e.g. total number of hits, number of delayed hits, number of reconstructed vertices in each cell)
- Make maps of the tracker showing the average value of some quantity in each cell (e.g. average drift radius per cell)
- Make maps of the calorimeter walls showing counts of some variable per calorimeter block (e.g. number of hits)
- Make maps of the calorimeter walls showing the average value of some quantity in each calorimeter block (e.g. average reconstructed energy per block, average hit time per block)

As you define the tuple yourself, it can include whatever information you like, and could be about simulated data, reconstructed quantities, or analysis-level quantities.

If you wish, you can use a configuration file to set some styling parameters for the plots, for example: a title, number of bins, maximum value. If no config file is provided or if there is no entry for a given branch, the tool will attempt to choose sensible defaults.

As well as storing images of the plots of each variable, the histograms are written to a ROOT file (ValidationHistograms.root), which you can use for further processing. Statistics are written to a text file (ValidationResults.txt)

If you give it a reference ROOT file, the tool will compare the branches with the same-named branch in the reference, producing ratio or pull plots, and writing goodness of fit statistics to a file.

## Usage
`./ValidationParser -i <data ROOT file> -r <reference ROOT file to compare to> -c <config file (optional)> -o <output directory (optional)> -t <temp directory (optional)>`

The root file should contain branches that you want to histogram. The naming convention is important and will be explained below. See the example ReconstructionValidationModule for details of how to make an ntuple with correctly named/formatted branches.

The configuration file (which will also be explained below) is optional, and allows you to set a title for the plots.

If a reference file is given it will (eventually) be used to make comparison and ratio plots, and calculate goodness of fit.

Plots images, histograms and a text file of results will be saved to the output directory (which will be created if it doesn't exist). If you don't specify an output folder, a directory will be created beneath the directory you are in when you run the tool. It will be neamed `plots_` followed by the name of your input ROOT file (minus the `.root` extension).

If your sample or reference files are large, the tool will need to write a temporary file (up to the size of those ROOT files) while it is working. You can specify a temp directory for those; if you don't, it will just put them into the same directory as your output plots. The temp file will be deleted when the tool completes.

The old syntax of
`./ValidationParser <data ROOT file> <config file (optional)>`
also still works, to maintain backwards compatibility.
## Ntuple format

There are currently 5 supported branch types, each denoted by a particular prefix. This information is taken from example ReconstructionValidationModule.

**Simple Histogram branches:** prefix: `h_`

Example: `h_total_calorimeter_energy`. The information in these branches will simply be histogrammed. A config file can be used to select the number of bins, and the minimum and maximum x values - otherwise they will be autogenerated. The config file can also give a title for the plots generated. If no title is specified, the parser will generate one by formatting the branch name, replacing underscores by spaces. For example, `h_calorimeter_hit_count` will get a default title of "Calorimeter hit count". An example config file line for a histogram variable is `h_cluster_count, Number of clusters, 10,0,5` which would tell you to entitle the plot for `h_cluster_count` "Number of clusters" and to use 10 bins, starting at 0 and going up to 5. Any of these fields can be left blank, or you can just not make an entry at all in the config file.

If you have provided a reference file, this will also make a plot showing the sample histogram (black points with error bars) superimposed on the scaled reference (red line with a pink error band). The Kolmogorov-Smirnov goodness of fit and chi-squared per degree of freedom will be calculated and written to an output text file.

**Tracker map branches:** prefix: `t_`

Example: `t_cell_hit_count`. This stores an encoded location (cell identifier) in the tracker. To use one of these branches, you MUST encode the location of each hit using the `EncodeLocation` function, then push it to a vector. In this example, `t_cell_hit_count` just stores the location of every Geiger hit but you could make a branch that stored something different - for example, only delayed hits.

This will produce a 2-d heat-map of the tracker, showing how many times each location was logged. For weighted maps, see the `tm_` prefix.

The config file allows you to set the title of this, as for the `h_` type branches.

If you have provided a reference file, this will also make a plot of the pull between the sample and the scaled reference. The Kolmogorov-Smirnov goodness of fit and chi-squared per degree of freedom will be calculated and written to an output text file. Any pulls over threshold (by default a difference of +/- 3 sigma) will be logged in the output file, as will the overall average pull.

**Tracker average branches:** prefix: `tm_`

Example: `tm_average_drift_radius.t_cell_hit_count`. This will map the mean of some value over the tracker. In order to do so, you need to push the value you want to average to a vector, in the same order that you pushed a location. The locations should be in the branch named after the `.` in the branch name.

This is confusing so here is an example. Let's say we have an event with 2 Geiger hits: one has a radius of 10mm, at tracker location (1,2). The other has a radius of 15mm, at tracker location (1,3). We will push two entries to the `tm_average_drift_radius.t_cell_hit_count branch`: 10 and 15. We will push two locations to the `t_cell_hit_count` branch - the encoded locations corresponding to (1,2) and (1,3). The parser will note that those radii (10 and 15) occurred at those locations ((1,2) and (1,3)) and include them when calculating the average radius recorded in the (1,2) and (1,3) cells.

The config file allows you to set the title of this, as for the `h_` type branches.

If you have provided a reference file, this will also make a plot of the pull between the sample and the scaled reference. The chi-squared per degree of freedom will also be calculated and written to an output text file. Any pulls over threshold (by default a difference of +/- 3 sigma) will be logged in the output file, as will the overall average pull and RMS of the pulls. These will be calculated by plotting the individual pulls in each cell and fitting to a Gaussian (that plot will also be saved).

The uncertainties on averaged branches are taken by finding the error on the mean. In the case that there is only 1 entry (or no entries), there will be insufficient data to calculate a pull or chi squared. The number of degrees of freedom for the chi squared will be decreased accordingly.

**Calorimeter branches:** prefix: `c_`

Example: `c_calorimeter_hit_map`.  This stores an encoded location (calorimeter identifier). To use one of these branches, you MUST encode the location of each hit using the `EncodeLocation` function, then push it to a vector. In this example, `c_calorimeter_hit_map` just stores the location of every calorimeter hit but you could make a branch that stored something different - for example, only hits associated with a track.

This will produce a 2-d heat-map of each calorimeter wall, showing how many times each location was logged. The 6 walls will be presented together as an image. For weighted maps, see the `cm_` prefix.

The config file allows you to set the title of this, as for the `h_` type branches.

If you have provided a reference file, this will also make a plot of the pull between the sample and the scaled reference for each wall. The total chi-squared per degree of freedom will be calculated and written to an output text file. Any pulls over threshold (by default a difference of +/- 3 sigma) will be logged in the output file, as will the overall average pull.

**Calorimeter average branches:** prefix: `cm_`

Example: `cm_average_calorimeter_energy.c_calorimeter_hit_map`. This will map the mean of some value over the 6 calorimeter walls. In order to do so, you need to push the value you want to average to a vector, in the same order that you pushed a location. The locations should be in the branch named after the `.` in the branch name.

This is confusing so here is an example. Let's say we have an event with 2 calorimeter hits: one is of 2 MeV, at location (3,4) on the Italian wall. The other has energy 1.5 MeV, at at location (2,7) on the French wall. We will push two entries to the `cm_average_calorimeter_energy.c_calorimeter_hit_map`: 2 and 1.5. We will push two locations to the `c_calorimeter_hit_map` branch - the encoded locations corresponding to (3,4) on the Italian wall and (2,7) on the French wall. The parser will note that those energies (2 and 1.5) were measured at those calorimeter locations, and include them in the average-energy calculation for those 2 particular calorimeter modules.

The config file allows you to set the title of this, as for the `h_` type branches.

If you have provided a reference file, this will also make a plot of the pull between the sample and the scaled reference. The chi-squared per degree of freedom will be calculated and written to an output text file. Any pulls over threshold (by default a difference of +/- 3 sigma) will be logged in the output file, as will the overall average pull. These will be calculated by plotting the individual pulls in each cell and fitting to a Gaussian (that plot will also be saved).

The uncertainties on averaged branches are taken by finding the error on the mean. In the case that there is only 1 entry (or no entries), there will be insufficient data to calculate a pull or chi squared. The number of degrees of freedom for the chi squared will be decreased accordingly.
