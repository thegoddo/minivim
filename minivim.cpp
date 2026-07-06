#include "minivim.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

MiniVim::MiniVim(const std::string &file) {
  x = y = 0;
  mode = 'n';
  status = " NORMAL";
  section = {};

  if (file.empty()) {
    filename = "untitled";
  } else {
    filename = file;
  }

  // 1. Initialize screen systems first
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);

  // 2. Safely initialize color setups
  start_color();
  use_default_colors();

  // 3. Populate buffer data
  open();
}

MiniVim::~MiniVim() {
  refresh();
  endwin();
}

void MiniVim::run() {
  while (mode != 'q') {
    update();
    statusline();
    print();
    int c = getch();
    input(c);
  }
}

void MiniVim::update() {
  switch (mode) {
  case 'n':
    status = " NORMAL";
    break;
  case 'i':
    status = " INSERT";
    break;
  case 'q':
    break;
  }

  section = " COLS: " + std::to_string(x) + " | ROWS: " + std::to_string(y) +
            " | FILE: " + filename;
}

void MiniVim::statusline() {
  if (mode == 'n') {
    init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
  } else {
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
  }

  attron(A_REVERSE);
  attron(A_BOLD);
  attron(COLOR_PAIR(1));

  for (int i{}; i < COLS; ++i) {
    mvprintw(LINES - 1, i, " ");
  }

  mvprintw(LINES - 1, 0, "%s", status.c_str());
  mvprintw(LINES - 1, COLS - section.length(), "%s", section.c_str());

  attroff(COLOR_PAIR(1));
  attroff(A_BOLD);
  attroff(A_REVERSE);
}

void MiniVim::input(int c) {
  // Arrow keys are persistent across modes
  switch (c) {
  case KEY_UP:
    up();
    return;
  case KEY_LEFT:
    left();
    return;
  case KEY_RIGHT:
    right();
    return;
  case KEY_DOWN:
    down();
    return;
  }

  switch (mode) {
  case 27:
  case 'n':
    switch (c) {
    case 'q':
      mode = 'q';
      break;
    case 'i':
      mode = 'i';
      break;
    case 'w':
      save();
      // Saved successfully; stays active in normal mode
      break;
    }
    break;

  case 'i':
    switch (c) {
    case 27: // ESC
      mode = 'n';
      break;
    case 127:
    case KEY_BACKSPACE:
      if (x == 0 && y > 0) {
        x = lines[y - 1].length();
        lines[y - 1] += lines[y];
        m_remove(y);
        up();
      } else if (x > 0) {
        lines[y].erase(--x, 1);
      }
      break;
    case KEY_DC: // Delete
      if (x == lines[y].length() && y != lines.size() - 1) {
        lines[y] += lines[y + 1];
        m_remove(y + 1);
      } else if (x < lines[y].length()) {
        lines[y].erase(x, 1);
      }
      break;
    case KEY_ENTER:
    case 10:
      if (x < lines[y].length()) {
        m_insert(lines[y].substr(x, lines[y].length() - x), y + 1);
        lines[y].erase(x, lines[y].length() - x);
      } else {
        m_insert("", y + 1);
      }
      x = 0;
      down();
      break;
    case 9: // Tab key handling
      lines[y].insert(x, 2, ' ');
      x += 2;
      break;
    default:
      lines[y].insert(x, 1, c);
      ++x;
      break;
    }
  }
}

void MiniVim::print() {
  for (size_t i{}; i < (size_t)LINES - 1; ++i) {
    if (i >= lines.size()) {
      move(i, 0);
      clrtoeol();
    } else {
      mvprintw(i, 0, "%s", lines[i].c_str());
    }
    clrtoeol();
  }
  move(y, x);
}

void MiniVim::m_remove(int number) { lines.erase(lines.begin() + number); }

std::string MiniVim::m_tabs(std::string &line) {
  std::size_t tab = line.find('\t');
  return tab == line.npos ? line : m_tabs(line.replace(tab, 1, "  "));
}

void MiniVim::m_insert(std::string line, int number) {
  line = m_tabs(line);
  lines.insert(lines.begin() + number, line);
}

void MiniVim::m_append(std::string &line) {
  line = m_tabs(line);
  lines.push_back(line);
}

void MiniVim::up() {
  if (y > 0) {
    --y;
  }
  if (x >= lines[y].length()) {
    x = lines[y].length();
  }
  move(y, x);
}

void MiniVim::left() {
  if (x > 0) {
    --x;
    move(y, x);
  }
}

void MiniVim::right() {
  if (x < lines[y].length()) {
    ++x;
    move(y, x);
  }
}

void MiniVim::down() {
  if (y < lines.size() - 1) {
    ++y;
  }
  if (x >= lines[y].length()) {
    x = lines[y].length();
  }
  move(y, x);
}

void MiniVim::open() {
  if (std::filesystem::exists(filename)) {
    std::ifstream ifile(filename);
    if (ifile.is_open()) {
      while (!ifile.eof()) {
        std::string buffer;
        std::getline(ifile, buffer);
        m_append(buffer);
      }
    } else {
      throw std::runtime_error("Cannot open file. Permission denied.");
    }
  } else {
    std::string str{};
    m_append(str);
  }
}

void MiniVim::save() {
  std::ofstream ofile(filename);
  if (ofile.is_open()) {
    for (size_t i{}; i < lines.size(); ++i) {
      ofile << lines[i] << '\n';
    }
    ofile.close();
  } else {
    refresh();
    endwin();
    throw std::runtime_error("Cannot save file. Permission denied.");
  }
}
