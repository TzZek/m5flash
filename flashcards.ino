#include <M5Core2.h>

// Struct to store flashcard information, including spaced repetition data
struct Flashcard {
    String question;
    String answer;
    float EF;              // Ease Factor
    int interval;          // Current interval in days
    int repetitions;       // Number of repetitions
    unsigned long nextReviewTime;
    bool inLearningPhase;  // To track if it's in the Learning Phase
    bool reviewed;         // Indicates if this card has been reviewed
};

// Initialize the flashcards array with all fields specified
Flashcard flashcards[] = {
    {"The Norwegian _______ is usually called the inventor of the paper clip.", "Johan Vaaler", 2.5f, 1, 0, 0UL, true, false},
    {"What is the capital of France?", "Paris", 2.5f, 1, 0, 0UL, true, false},
    {"Which planet is known as the Red Planet?", "Mars", 2.5f, 1, 0, 0UL, true, false}
};

int currentCardIndex = 0;
int totalCards = sizeof(flashcards) / sizeof(flashcards[0]);
bool showingAnswer = false;
bool allReviewed = false;

// Button Coordinates
int buttonHeight = 50;
int buttonPadding = 10;
int buttonY = M5.Lcd.height() - buttonHeight - buttonPadding;

// Feedback Grades
enum FeedbackGrade { FORGOT = 1, PARTIALLY_RECALLED = 2, RECALLED_WITH_EFFORT = 3, EASILY_RECALLED = 4 };

// Function to split a long string into lines that fit the screen width
void drawWrappedText(String text, int x, int y, int width) {
    int cursorX = x;
    int cursorY = y;
    String currentLine = "";

    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        currentLine += c;

        int lineWidth = M5.Lcd.textWidth(currentLine);
        if (lineWidth >= width || c == '\n') {
            M5.Lcd.drawString(currentLine, cursorX, cursorY);
            currentLine = "";
            cursorY += M5.Lcd.fontHeight();
        }
    }
    if (currentLine.length() > 0) {
        M5.Lcd.drawString(currentLine, cursorX, cursorY);
    }
}

// Function to update flashcard based on feedback grade
void updateFlashcard(Flashcard &card, int grade) {
    if (card.inLearningPhase) {
        // Learning Phase behavior
        if (grade == FORGOT) {
            card.repetitions = 0;
            card.interval = 1;  // Reset to initial step
        } else if (grade == PARTIALLY_RECALLED) {
            card.repetitions = 0;  // Stay in the current phase
            card.interval = card.interval / 2;
        } else if (grade == RECALLED_WITH_EFFORT || grade == EASILY_RECALLED) {
            card.repetitions += 1;
            if (grade == EASILY_RECALLED || card.repetitions >= 3) {
                card.inLearningPhase = false;  // Move to Exponential Phase
                card.interval = card.interval * card.EF;  // Calculate first interval
            } else {
                card.interval = 6;  // Example: move to next learning step
            }
        }
    } else {
        // Exponential Phase behavior
        if (grade == FORGOT) {
            card.EF -= 0.2;
            if (card.EF < 1.3) card.EF = 1.3;  // EF can't go below 1.3
            card.interval = max(1, int(card.interval * 0.1));  // Lapse interval
            card.inLearningPhase = true;  // Move back to Learning Phase
        } else if (grade == PARTIALLY_RECALLED) {
            card.EF -= 0.15;
            if (card.EF < 1.3) card.EF = 1.3;
            card.interval = int(card.interval * 1.2);
        } else if (grade == RECALLED_WITH_EFFORT) {
            card.interval = int(card.interval * card.EF);
        } else if (grade == EASILY_RECALLED) {
            card.EF += 0.15;
            card.interval = int(card.interval * card.EF * 1.3);  // Apply Easy Bonus
        }
    }

    // Update next review time
    card.nextReviewTime = millis() + (card.interval * 86400000);  // Convert days to milliseconds
    card.reviewed = true;  // Mark as reviewed
}

// Function to handle button clicks
void handleButtonClick(int grade) {
    updateFlashcard(flashcards[currentCardIndex], grade);
    showingAnswer = false;
    showNextFlashcard();
}

// Function to find the next flashcard to show
void showNextFlashcard() {
    unsigned long currentTime = millis();
    int nextCardIndex = -1;
    unsigned long nearestTime = ULONG_MAX;
    allReviewed = true;

    // Find the next card that is due for review
    for (int i = 0; i < totalCards; i++) {
        if (!flashcards[i].reviewed || flashcards[i].nextReviewTime <= currentTime) {
            allReviewed = false;
            if (flashcards[i].nextReviewTime <= currentTime && flashcards[i].nextReviewTime < nearestTime) {
                nearestTime = flashcards[i].nextReviewTime;
                nextCardIndex = i;
            }
        }
    }

    // If a card is found, show it; otherwise, display the next review time message
    if (nextCardIndex != -1) {
        currentCardIndex = nextCardIndex;
        displayFlashcardQuestion();
    } else {
        displayReviewTimeMessage(nearestTime);
    }
}

// Function to display the current question
void displayFlashcardQuestion() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextDatum(TL_DATUM);
    drawWrappedText(flashcards[currentCardIndex].question, 10, 40, M5.Lcd.width() - 20);
    M5.Lcd.fillRect(80, buttonY, 160, buttonHeight, 0x444444);  // Darker grey color
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawCentreString("Show Answer", 160, buttonY + 20, 2);
}

// Function to display the current answer
void displayFlashcardAnswer() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(BLUE);
    M5.Lcd.setTextDatum(TL_DATUM);
    drawWrappedText(flashcards[currentCardIndex].answer, 10, 40, M5.Lcd.width() - 20);

    // Draw feedback buttons
    M5.Lcd.setTextColor(RED);
    M5.Lcd.fillRect(10, buttonY, 100, buttonHeight, BLACK);
    M5.Lcd.drawRect(10, buttonY, 100, buttonHeight, WHITE);
    M5.Lcd.drawCentreString("Hard", 60, buttonY + 20, 2);

    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.fillRect(120, buttonY, 80, buttonHeight, BLACK);
    M5.Lcd.drawRect(120, buttonY, 80, buttonHeight, WHITE);
    M5.Lcd.drawCentreString("Med", 160, buttonY + 20, 2);

    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.fillRect(210, buttonY, 100, buttonHeight, BLACK);
    M5.Lcd.drawRect(210, buttonY, 100, buttonHeight, WHITE);
    M5.Lcd.drawCentreString("Easy", 260, buttonY + 20, 2);
}

// Function to display the next review time message
void displayReviewTimeMessage(unsigned long nearestTime) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextDatum(TL_DATUM);

    // Calculate the time remaining
    unsigned long currentTime = millis();
    unsigned long timeRemaining = nearestTime - currentTime;
    int days = timeRemaining / 86400000;
    int hours = (timeRemaining % 86400000) / 3600000;
    int minutes = (timeRemaining % 3600000) / 60000;

    // Display the message with proper spacing
    drawWrappedText("All flashcards reviewed! Next review in:", 10, 40, M5.Lcd.width() - 20);
    String timeMessage = String(days) + "d " + String(hours) + "h " + String(minutes) + "m";
    M5.Lcd.drawCentreString(timeMessage, M5.Lcd.width() / 2, 100, 2);
}

void setup() {
    M5.begin();
    M5.Lcd.setTextFont(2);
    showNextFlashcard();
}

void loop() {
    M5.update();

    // Check if the screen is touched
    if (M5.Touch.ispressed()) {
        int touchX = M5.Touch.getPressPoint().x;
        int touchY = M5.Touch.getPressPoint().y;

        // Check which button was pressed
        if (touchY > buttonY && touchY < buttonY + buttonHeight) {
            if (showingAnswer) {
                if (touchX >= 10 && touchX < 110) {
                    handleButtonClick(FORGOT);
                } else if (touchX >= 120 && touchX < 200) {
                    handleButtonClick(PARTIALLY_RECALLED);
                } else if (touchX >= 210 && touchX < 310) {
                    handleButtonClick(EASILY_RECALLED);
                }
            } else {
                if (touchX >= 80 && touchX < 240) {
                    showingAnswer = true;
                    displayFlashcardAnswer();
                }
            }
        }
    }
}
