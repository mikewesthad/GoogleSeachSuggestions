#line 1 "GoogleSearchSuggestions"

#include "ofMain.h"
#include "ofxXmlSettings.h"


class ofApp: public ofBaseApp
{
public:

#line 2 "GoogleSearchSuggestions"
// =============================================================================
//
// Code from a project involving mining google search suggestions
//
// This visualizer will asynchronously query 50 top level domain names (.com, 
// .uk, etc.) for the search suggestions for a given phrase.
//
// Type to modify the search term.
// Press enter to begin looking for search suggestions
// Press F1 to save the search suggestions for the current search term to a 
//  plaintext file within bin/data.
// 
// =============================================================================


int numRegions;
vector< string > regions;
vector< string > topLevelDomains;
string lastSearchTerm;
string searchTerm;
vector< string > suggestionResults;
vector< string > responsesLoaded;
ofTrueTypeFont searchFont;
ofTrueTypeFont resultsFont;
float resultsCharacterHeight;
float resultsSpacing;
float resultsIndention;
bool isFirstKeyPress;
bool hasSearchChanged;
bool isSearching;

void setup() {
    ofSetWindowShape(1200, 1000);
    
    ofRegisterURLNotification(this);

    // Load in the regions and their top level domains (e.g. "US" and ".com") to be crawled
    //  The file TopLevelDomains.txt should be in the bin\data folder and,
    //  each line should look like "US,.com" without any whitespace
    ofBuffer topLevelDomainsBuffer = ofBufferFromFile("TopLevelDomains.txt");
    while (!topLevelDomainsBuffer.isLastLine()) {
        string line = topLevelDomainsBuffer.getNextLine();
        std::vector<string> split = ofSplitString(line, ",");
        string region = split[0];
        ofStringReplace(region, " ", "");
        string topLevelDomain = split[1];
        ofStringReplace(topLevelDomain, "\n", "");
        regions.push_back(region);
        topLevelDomains.push_back(topLevelDomain);
    }
    numRegions = regions.size();

    // Phrase would you like google to complete
    searchTerm = "is it normal";
    lastSearchTerm = searchTerm;
    isFirstKeyPress = true;
    hasSearchChanged = false;
    searchFont.loadFont("verdanab.ttf", 30);
    resultsFont.loadFont("verdana.ttf", 10);

    resultsCharacterHeight = resultsFont.stringHeight("pgyOTP,.");
    resultsSpacing = 0.2;
    resultsIndention = resultsFont.stringWidth("pppp");

    isSearching = false;	
}

void update() {
    // Check if it is time to initiate a new search
    if (hasSearchChanged && searchTerm!="") {
        isSearching = true;
        lastSearchTerm = searchTerm;
        hasSearchChanged = false;
        findUniqueGoogleCompletions(searchTerm);
    }
}

void draw() {
	ofBackground(25);

    // Draw the current google search text
    ofSetColor(255);
    string displayText = searchTerm + "...";
    ofRectangle bboxSearch = searchFont.getStringBoundingBox(displayText, 0, 0);
    bboxSearch.height = searchFont.stringHeight("pgyOTP,."); // Override height so it doesn't shift around when letters change
    bboxSearch.x = ofGetWidth()/2.0 - bboxSearch.width/2.0;
    bboxSearch.y = ofGetHeight()/2.0 - bboxSearch.height/2.0;
    searchFont.drawString(displayText, bboxSearch.x, bboxSearch.getBottom());

    // Display search progress
    if (isSearching) {
        ofSetColor(225);
        float searchProgress = responsesLoaded.size() / float(numRegions) * 100.0;
        string displayText = "(Search progress for \"" + searchTerm + "\": " + ofToString(searchProgress, 1) + "%)";
        ofRectangle bboxProgress = resultsFont.getStringBoundingBox(displayText, 0, 0);
        bboxProgress.height = resultsCharacterHeight;
        bboxProgress.x = ofGetWidth()/2.0 - bboxProgress.width/2.0;
        bboxProgress.y = bboxSearch.getBottom() + resultsCharacterHeight;
        resultsFont.drawString(displayText, bboxProgress.x, bboxProgress.getBottom());
    }

    // Draw the search results
    ofSetColor(255);
    float x = 10;
    float y = 10 + resultsCharacterHeight/2.0;
    for (int i=0; i<suggestionResults.size(); i++) {
        string suggestion = suggestionResults[i];
        ofRectangle bbox = resultsFont.getStringBoundingBox(suggestion, x, y);
        resultsFont.drawString(suggestion, bbox.x, bbox.y+resultsCharacterHeight);
        y += (1+resultsSpacing) * resultsCharacterHeight;
        if (y >= (ofGetScreenHeight() - resultsCharacterHeight)) {
            x += 400;
            y = 10 + resultsCharacterHeight/2.0;
        }
    }
	
}


void findUniqueGoogleCompletions(string searchTerm) {
    // Kill any existing requests
    ofRemoveAllURLRequests();
    responsesLoaded.clear();

    // Clear the current results
    suggestionResults.clear();

    // Convert it to url by replacing spaces with '+'
    string urlSearchTerm = searchTerm;
    ofStringReplace(urlSearchTerm, " ", "+");

    // Send an asynchronous request to google for each top level domain name
    for (int r=0; r<numRegions; ++r) {
        string url = "http://google" + topLevelDomains[r] + "/complete/search?output=toolbar&q=" + urlSearchTerm;
        ofLoadURLAsync(url, ofToString(r));
    }
}

void urlResponse(ofHttpResponse& response) {
    responsesLoaded.push_back(response.request.name);
    if (response.status == 200) processSearchSuggestions(response);
    else cout << response.status << " " << response.error << endl;
    if (responsesLoaded.size() == numRegions) isSearching = false;
}

void processSearchSuggestions(ofHttpResponse& response) {
    // The returned information is an xml that looks like this:
    // <toplevel>
    //  <CompleteSuggestion>
    //      <suggestion data="is it bad if you sleep with a tampon"/>
    //  </CompleteSuggestion>
    //  ...
    // </toplevel>

    // Load the xml data into an xml object
    ofxXmlSettings xmlCompletions;
    xmlCompletions.loadFromBuffer(response.data);

    // Enter the root level of the returned xml which is called 'toplevel'
    xmlCompletions.pushTag("toplevel");

    // Find out how many CompleteSuggestion tags there are
    int numCompletions = xmlCompletions.getNumTags("CompleteSuggestion");

    // Loop through each CompleteSuggestion tag and pull out the data
    // attribute from the suggestion tag
    for (int i=0; i<numCompletions; i++) {
        xmlCompletions.pushTag("CompleteSuggestion", i);
        string completion = xmlCompletions.getAttribute("suggestion", "data", "");
        bool isUnique = find(suggestionResults.begin(), suggestionResults.end(), completion) == suggestionResults.end();
        if (isUnique) suggestionResults.push_back(completion);
        xmlCompletions.popTag();
    }
}

void saveResults() {
    ofFile outFile;
    string filename = lastSearchTerm + "_" + ofGetTimestampString() + ".txt";
    ofStringReplace(filename, " ", "-");
    ofStringReplace(filename, "*", ".");
    ofStringReplace(filename, "?", ".");
    outFile.open(filename, ofFile::WriteOnly, false);
    outFile << "## " << lastSearchTerm << "...\n\n" << "Search suggestions:";
    for (int i=0; i<suggestionResults.size(); i++) outFile << "\n\t" << suggestionResults[i];
    outFile << "\n\nRetrieved on:\n\t" << ofGetTimestampString("%W, %B %d, %Y at %h:%M:%S%a\n\n");
    outFile << "Domains searched:\n\t" << regions[0];
    for (int i=1; i<regions.size(); i++) outFile << ", " << regions[i];
    outFile.close();
}

void keyPressed(int k){
    // If this is the first key press, delete the default search term
    if(isFirstKeyPress && k!=OF_KEY_RETURN && k!=OF_KEY_F1) {
        searchTerm = "";
        isFirstKeyPress = false;
    }
    
	if(k == OF_KEY_DEL) searchTerm = "";
	else if (k == OF_KEY_BACKSPACE) searchTerm = searchTerm.substr(0, searchTerm.length()-1);
	else if (k == OF_KEY_RETURN) hasSearchChanged = true;
	else {
	    bool isNumeric = k >= '0' && k <= '9';
        bool isAlpha = (k >= 'A' && k <= 'Z') || (k >= 'a' && k <= 'z');
        bool isSpecialAllowed = (k == '\'') || (k == '*') || (k == ',') || 
                                (k == '"') || (k == '-') || (k == ' ') || 
                                (k == '?');
        if (isNumeric || isAlpha || isSpecialAllowed) searchTerm.append(1, (char)k);
        else if (k == OF_KEY_F1 && suggestionResults.size() != 0) saveResults();
	}
}


};

int main()
{
    ofSetupOpenGL(320, 240, OF_WINDOW);
    ofRunApp(new ofApp());
}

