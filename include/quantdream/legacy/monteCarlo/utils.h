#include <Eigen/Dense>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cmath>
#include <limits>
#include <iostream>

void plotPortfolioLosses(const Eigen::MatrixXd& riskMeasureMatrix,
                         const std::string folderName = "MonteCarlo",
                         const std::string fileName = "monte_carlo_losses",
                         const int nbins = 100) {
    const int nCols = static_cast<int>(riskMeasureMatrix.cols());
    const int nRows = static_cast<int>(riskMeasureMatrix.rows());

    // Save each column to a separate file and compute min/max
    std::vector<double> mins(nCols, std::numeric_limits<double>::max());
    std::vector<double> maxs(nCols, std::numeric_limits<double>::lowest());

    for (int col = 0; col < nCols; ++col) {
        std::string filename = "losses_col" + std::to_string(col) + ".dat";
        std::ofstream ofs(filename);
        for (int i = 0; i < nRows; ++i) {
            double val = riskMeasureMatrix(i, col);
            ofs << val << "\n";
            if (val < mins[col]) mins[col] = val;
            if (val > maxs[col]) maxs[col] = val;
        }
    }

    // Create gnuplot script for subplots
    std::ofstream gp(fileName + ".gp");
    int nRowsFig = std::ceil(std::sqrt(nCols));
    int nColsFig = std::ceil(double(nCols) / nRowsFig);

    gp << "set style fill solid 0.5 border\n";
    gp << "set multiplot layout " << nRowsFig << "," << nColsFig
       << " title 'Loss Distributions'\n";
    gp << "bin(x,width)=width*floor(x/width)\n";

    for (int col = 0; col < nCols; ++col) {
        std::string label = (col == nCols - 1)
            ? "Portfolio Losses"
            : "Asset " + std::to_string(col + 1);

        double range = maxs[col] - mins[col];
        if (range <= 0) range = 1.0; // fallback
        double binwidth = range / static_cast<double>(nbins);

        gp << "set title '" << label << "'\n";
        gp << "bw=" << binwidth << "\n";
        gp << "plot 'losses_col" << col
           << ".dat' using (bin($1,bw)):(1.0) smooth freq with boxes notitle\n";
    }

    gp << "unset multiplot\n";
    gp.close();
    // Run gnuplot interactively, keep window open
    std::system("gnuplot -persist -e \"set terminal qt size 1200,800\" plot.gp");

    // And save file
    std::string instructions = "gnuplot -e \"set terminal pngcairo size 1200,800; "
                               "set output '../standalone/images/" + folderName +
                               fileName + ".png'\" " + fileName + ".gp";
    std::system(instructions.c_str());
}
