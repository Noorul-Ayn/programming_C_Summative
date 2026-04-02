#!/bin/bash

# ============================================================
#  Linux Server Health Monitor v1.0
#  Monitors CPU, Memory, Disk, and Processes
#  Features: Alerts, Logging, Interactive Menu
# ============================================================

# --- Configuration ---
LOG_FILE="$HOME/server_health.log"
CONFIG_FILE="$HOME/monitor_config.cfg"
MONITOR_INTERVAL=60
MONITORING=false
MONITOR_PID=""

# --- Default Thresholds ---
CPU_THRESHOLD=80
MEM_THRESHOLD=80
DISK_THRESHOLD=90

# ============================================================
#  COLOURS
# ============================================================
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# ============================================================
#  DEPENDENCY CHECK
# ============================================================
check_dependencies() {
  local deps=("top" "free" "df" "ps" "awk" "grep" "date")
  for cmd in "${deps[@]}"; do
    if ! command -v "$cmd" &>/dev/null; then
      echo -e "${RED}[ERROR] Required command '$cmd' not found. Exiting.${RESET}"
      exit 1
    fi
  done
}

# ============================================================
#  LOAD / SAVE CONFIG
# ============================================================
load_config() {
  if [[ -f "$CONFIG_FILE" ]]; then
    source "$CONFIG_FILE"
  fi
}

save_config() {
  cat > "$CONFIG_FILE" <<EOF
CPU_THRESHOLD=$CPU_THRESHOLD
MEM_THRESHOLD=$MEM_THRESHOLD
DISK_THRESHOLD=$DISK_THRESHOLD
MONITOR_INTERVAL=$MONITOR_INTERVAL
EOF
  echo -e "${GREEN}[OK] Configuration saved.${RESET}"
}

# ============================================================
#  LOGGING
# ============================================================
log() {
  local level="$1"
  local message="$2"
  local timestamp
  timestamp=$(date '+%Y-%m-%d %H:%M:%S')
  echo "[$timestamp] [$level] $message" >> "$LOG_FILE"
}

# ============================================================
#  GET METRICS
# ============================================================
get_cpu() {
  # top -bn1: run once in batch mode, extract CPU idle, subtract from 100
  local idle
  idle=$(top -bn1 | grep "Cpu(s)" | awk '{print $8}' | tr -d '%,')
  # fallback for different top formats
  if [[ -z "$idle" ]]; then
    idle=$(top -bn1 | grep "%Cpu" | awk '{print $8}')
  fi
  local usage
  usage=$(echo "100 - ${idle:-0}" | bc 2>/dev/null || echo "0")
  echo "$usage"
}

get_memory() {
  # Returns used memory percentage
  free | awk '/^Mem:/ {
    total=$2; used=$3
    printf "%.1f", (used/total)*100
  }'
}

get_memory_details() {
  free -h | awk '/^Mem:/ {
    printf "Total: %s | Used: %s | Free: %s", $2, $3, $4
  }'
}

get_disk() {
  # Returns highest disk usage percentage across all mount points
  df -h | awk 'NR>1 {
    gsub(/%/,"",$5)
    if ($5+0 > max) max=$5
  } END {print max}'
}

get_disk_details() {
  df -h | awk 'NR==1 || /^\// {printf "  %-20s %5s %5s %5s %5s\n", $1,$2,$3,$4,$5}'
}

get_processes() {
  ps aux --no-header | wc -l
}

get_top_processes() {
  ps aux --no-header --sort=-%cpu | head -5 | \
    awk '{printf "  %-10s %5s%% %5s%% %s\n", $1, $3, $4, $11}'
}

# ============================================================
#  DISPLAY DASHBOARD
# ============================================================
display_health() {
  local cpu mem disk procs
  cpu=$(get_cpu)
  mem=$(get_memory)
  disk=$(get_disk)
  procs=$(get_processes)

  clear
  echo -e "${BOLD}${CYAN}"
  echo "╔══════════════════════════════════════════╗"
  echo "║      SERVER HEALTH MONITOR v1.0          ║"
  echo "╚══════════════════════════════════════════╝"
  echo -e "${RESET}"
  echo -e "  ${BOLD}Timestamp:${RESET} $(date '+%Y-%m-%d %H:%M:%S')"
  echo ""

  # CPU
  local cpu_colour=$GREEN
  [[ $(echo "$cpu > $CPU_THRESHOLD" | bc) -eq 1 ]] && cpu_colour=$RED
  [[ $(echo "$cpu > $((CPU_THRESHOLD - 10))" | bc) -eq 1 && \
     $(echo "$cpu <= $CPU_THRESHOLD" | bc) -eq 1 ]] && cpu_colour=$YELLOW
  echo -e "  ${BOLD}CPU Usage:${RESET}    ${cpu_colour}${cpu}%${RESET}  (Threshold: ${CPU_THRESHOLD}%)"

  # Memory
  local mem_colour=$GREEN
  [[ $(echo "$mem > $MEM_THRESHOLD" | bc) -eq 1 ]] && mem_colour=$RED
  [[ $(echo "$mem > $((MEM_THRESHOLD - 10))" | bc) -eq 1 && \
     $(echo "$mem <= $MEM_THRESHOLD" | bc) -eq 1 ]] && mem_colour=$YELLOW
  echo -e "  ${BOLD}Memory Usage:${RESET} ${mem_colour}${mem}%${RESET}  (Threshold: ${MEM_THRESHOLD}%)"
  echo -e "               $(get_memory_details)"

  # Disk
  local disk_colour=$GREEN
  [[ "$disk" -gt "$DISK_THRESHOLD" ]] && disk_colour=$RED
  [[ "$disk" -gt "$((DISK_THRESHOLD - 10))" && "$disk" -le "$DISK_THRESHOLD" ]] && disk_colour=$YELLOW
  echo -e "  ${BOLD}Disk Usage:${RESET}   ${disk_colour}${disk}%${RESET}  (Threshold: ${DISK_THRESHOLD}%)"
  echo ""
  echo -e "  ${BOLD}Disk Details:${RESET}"
  get_disk_details

  # Processes
  echo ""
  echo -e "  ${BOLD}Active Processes:${RESET} $procs"
  echo ""
  echo -e "  ${BOLD}Top 5 by CPU:${RESET}"
  echo -e "  ${CYAN}USER        CPU%   MEM%  COMMAND${RESET}"
  get_top_processes

  echo ""
  echo "──────────────────────────────────────────"

  # Check thresholds and alert
  check_alerts "$cpu" "$mem" "$disk"
}

# ============================================================
#  THRESHOLD ALERTS
# ============================================================
check_alerts() {
  local cpu="$1" mem="$2" disk="$3"
  local alert=false

  if (( $(echo "$cpu > $CPU_THRESHOLD" | bc) )); then
    echo -e "  ${RED}⚠ ALERT: CPU usage ${cpu}% exceeds threshold ${CPU_THRESHOLD}%${RESET}"
    log "ALERT" "CPU usage ${cpu}% exceeds threshold ${CPU_THRESHOLD}%"
    alert=true
  fi

  if (( $(echo "$mem > $MEM_THRESHOLD" | bc) )); then
    echo -e "  ${RED}⚠ ALERT: Memory usage ${mem}% exceeds threshold ${MEM_THRESHOLD}%${RESET}"
    log "ALERT" "Memory usage ${mem}% exceeds threshold ${MEM_THRESHOLD}%"
    alert=true
  fi

  if [[ "$disk" -gt "$DISK_THRESHOLD" ]]; then
    echo -e "  ${RED}⚠ ALERT: Disk usage ${disk}% exceeds threshold ${DISK_THRESHOLD}%${RESET}"
    log "ALERT" "Disk usage ${disk}% exceeds threshold ${DISK_THRESHOLD}%"
    alert=true
  fi

  if [[ "$alert" == false ]]; then
    echo -e "  ${GREEN}✔ All systems within normal limits.${RESET}"
    log "INFO" "Health check OK — CPU:${cpu}% MEM:${mem}% DISK:${disk}%"
  fi
}

# ============================================================
#  CONFIGURE THRESHOLDS
# ============================================================
configure_thresholds() {
  echo -e "\n${BOLD}${CYAN}Configure Thresholds${RESET}"
  echo "Current values: CPU=${CPU_THRESHOLD}%  MEM=${MEM_THRESHOLD}%  DISK=${DISK_THRESHOLD}%"
  echo ""

  read -rp "  Enter new CPU threshold (1-100) [${CPU_THRESHOLD}]: " input
  if [[ "$input" =~ ^[0-9]+$ ]] && (( input >= 1 && input <= 100 )); then
    CPU_THRESHOLD=$input
  elif [[ -n "$input" ]]; then
    echo -e "${RED}[ERROR] Invalid value. Keeping ${CPU_THRESHOLD}%.${RESET}"
  fi

  read -rp "  Enter new Memory threshold (1-100) [${MEM_THRESHOLD}]: " input
  if [[ "$input" =~ ^[0-9]+$ ]] && (( input >= 1 && input <= 100 )); then
    MEM_THRESHOLD=$input
  elif [[ -n "$input" ]]; then
    echo -e "${RED}[ERROR] Invalid value. Keeping ${MEM_THRESHOLD}%.${RESET}"
  fi

  read -rp "  Enter new Disk threshold (1-100) [${DISK_THRESHOLD}]: " input
  if [[ "$input" =~ ^[0-9]+$ ]] && (( input >= 1 && input <= 100 )); then
    DISK_THRESHOLD=$input
  elif [[ -n "$input" ]]; then
    echo -e "${RED}[ERROR] Invalid value. Keeping ${DISK_THRESHOLD}%.${RESET}"
  fi

  read -rp "  Enter monitoring interval in seconds [${MONITOR_INTERVAL}]: " input
  if [[ "$input" =~ ^[0-9]+$ ]] && (( input >= 5 )); then
    MONITOR_INTERVAL=$input
  elif [[ -n "$input" ]]; then
    echo -e "${RED}[ERROR] Invalid value. Must be >= 5. Keeping ${MONITOR_INTERVAL}s.${RESET}"
  fi

  save_config
}

# ============================================================
#  VIEW LOGS
# ============================================================
view_logs() {
  echo -e "\n${BOLD}${CYAN}Activity Log — $LOG_FILE${RESET}\n"
  if [[ -f "$LOG_FILE" ]]; then
    tail -50 "$LOG_FILE"
    echo ""
    echo -e "  (Showing last 50 entries)"
  else
    echo -e "${YELLOW}  No log file found.${RESET}"
  fi
}

# ============================================================
#  CLEAR LOGS
# ============================================================
clear_logs() {
  read -rp "  Are you sure you want to clear the log file? (y/n): " confirm
  if [[ "$confirm" == "y" || "$confirm" == "Y" ]]; then
    > "$LOG_FILE"
    echo -e "${GREEN}[OK] Log file cleared.${RESET}"
    log "INFO" "Log file cleared by user."
  else
    echo -e "${YELLOW}  Cancelled.${RESET}"
  fi
}

# ============================================================
#  BACKGROUND MONITORING LOOP
# ============================================================
monitor_loop() {
  log "INFO" "Background monitoring started. Interval: ${MONITOR_INTERVAL}s"
  while true; do
    local cpu mem disk
    cpu=$(get_cpu)
    mem=$(get_memory)
    disk=$(get_disk)
    log "INFO" "Health check — CPU:${cpu}% MEM:${mem}% DISK:${disk}%"

    # Alert checks
    if (( $(echo "$cpu > $CPU_THRESHOLD" | bc) )); then
      log "ALERT" "CPU usage ${cpu}% exceeds threshold ${CPU_THRESHOLD}%"
    fi
    if (( $(echo "$mem > $MEM_THRESHOLD" | bc) )); then
      log "ALERT" "Memory usage ${mem}% exceeds threshold ${MEM_THRESHOLD}%"
    fi
    if [[ "$disk" -gt "$DISK_THRESHOLD" ]]; then
      log "ALERT" "Disk usage ${disk}% exceeds threshold ${DISK_THRESHOLD}%"
    fi

    sleep "$MONITOR_INTERVAL"
  done
}

start_monitoring() {
  if [[ "$MONITORING" == true ]]; then
    echo -e "${YELLOW}  Monitoring is already running (PID: $MONITOR_PID).${RESET}"
    return
  fi
  monitor_loop &
  MONITOR_PID=$!
  MONITORING=true
  echo -e "${GREEN}[OK] Background monitoring started (PID: $MONITOR_PID, Interval: ${MONITOR_INTERVAL}s).${RESET}"
  log "INFO" "Monitoring started by user. PID: $MONITOR_PID"
}

stop_monitoring() {
  if [[ "$MONITORING" == false ]]; then
    echo -e "${YELLOW}  Monitoring is not running.${RESET}"
    return
  fi
  kill "$MONITOR_PID" 2>/dev/null
  MONITORING=false
  MONITOR_PID=""
  echo -e "${GREEN}[OK] Monitoring stopped.${RESET}"
  log "INFO" "Monitoring stopped by user."
}

# ============================================================
#  MAIN MENU
# ============================================================
print_menu() {
  echo -e "\n${BOLD}${CYAN}"
  echo "╔══════════════════════════════════════════╗"
  echo "║         SERVER HEALTH MONITOR            ║"
  echo "╠══════════════════════════════════════════╣"
  echo "║  1. Display current system health        ║"
  echo "║  2. Configure monitoring thresholds      ║"
  echo "║  3. View activity logs                   ║"
  echo "║  4. Clear logs                           ║"
  echo "║  5. Start background monitoring          ║"
  echo "║  6. Stop background monitoring           ║"
  echo "║  7. Exit                                 ║"
  echo "╚══════════════════════════════════════════╝"
  echo -e "${RESET}"
  local status_colour=$RED
  local status_text="STOPPED"
  [[ "$MONITORING" == true ]] && status_colour=$GREEN && status_text="RUNNING (PID: $MONITOR_PID)"
  echo -e "  Monitor Status: ${status_colour}${status_text}${RESET}"
  echo ""
}

# ============================================================
#  ENTRY POINT
# ============================================================
main() {
  check_dependencies
  load_config

  log "INFO" "Server Health Monitor started."

  while true; do
    print_menu
    read -rp "  Enter choice [1-7]: " choice

    case "$choice" in
      1) display_health ;;
      2) configure_thresholds ;;
      3) view_logs ;;
      4) clear_logs ;;
      5) start_monitoring ;;
      6) stop_monitoring ;;
      7)
        stop_monitoring
        log "INFO" "Server Health Monitor exited."
        echo -e "${GREEN}  Goodbye!${RESET}"
        exit 0
        ;;
      *)
        echo -e "${RED}  [ERROR] Invalid option '$choice'. Please enter 1-7.${RESET}"
        ;;
    esac

    echo ""
    read -rp "  Press Enter to continue..."
  done
}

main
