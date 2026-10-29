import { Immutable, MessageEvent, PanelExtensionContext } from "@foxglove/extension";
import { ReactElement, useEffect, useLayoutEffect, useRef, useState } from "react";
import { createRoot } from "react-dom/client";

import "./fault-card.css";

function date_to_str(date: Date): string {
  const time = date.toLocaleTimeString("en-US", {
    hour12: false,
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
  const ms = String(date.getMilliseconds()).padStart(3, "0");
  const timezone =
    new Intl.DateTimeFormat("en-US", {
      timeZoneName: "short",
    })
      .formatToParts(date)
      .find((part) => part.type === "timeZoneName")?.value ?? "???";

  return `${time}.${ms} ${timezone}`;
}

const SENSORS = [
  "ADC_SDC_DASH",
  "ADC_SDC_BSPD",
  "ADC_SDC_MASTER_PANEL",
  "ADC_SDC_BMS",
  "ADC_SDC_VCU",
  "ADC_BRAKE_PRESSURE",
  "ADC_BRAKE_PEDAL",
  "ADC_THROTTLE",
  "ADC_WHEEL_SPEED_FL",
  "ADC_WHEEL_SPEED_FR",
  "ADC_WHEEL_SPEED_RL",
  "ADC_WHEEL_SPEED_RR",
  "ADC_STEERING_ANGLE",
  "ADC_ACCEL_X",
  "ADC_ACCEL_Y",
];

const FAULT_TYPE = [
  "UNSTABLE_SIGNAL",
  "NEAR_OUT_OF_RANGE",
  "OUT_OF_RANGE",
  "BRAKE_DEVIATION",
  "SDC_IMPLAUSIBILITY",
];

const SEVERITY: { [key: string]: string } = {};
enum Severity {
  Warning = 0,
  Error = 1,
}
SEVERITY[Severity.Warning] = "Warning";
SEVERITY[Severity.Error] = "Error";

interface CanMessage {
  fault_n: number;
  fault_type: number;
  fault_severity: 0 | 1;
  fault_arg: number;
  fault_detected_at: number;
}

type FaultProps = { name: string; value: string }[];

function parse_arg(type: number, arg: number): FaultProps | undefined {
  if (type === 0) {
    return [{ name: "", value: SENSORS[arg & 0xff]! }];
  } else if (type === 1 || type === 2) {
    return [
      { name: "", value: SENSORS[arg & 0xff]! },
      { name: "", value: `${((arg >> 8) & 0xffff) / 1000}V` },
      // { name: "", value: `${arg}` },
    ];
  } else if (type === 4 || type === 5) {
    return [];
  }

  return undefined;
}

interface MessageFault {
  type: number;
  severity: 0 | 1;
  arg: number;
  detected_at: number;
}

interface Fault {
  type: number;
  severity: number;
  props: FaultProps;
  acknowledged: boolean;
  detected_at: Date;
}

const CARD_ACCENT_ERROR = "#a32222";
const CARD_BACKGROUND_ERROR = "#170B0B";

const CARD_ACCENT_WARNING = "#754E00";
const CARD_BACKGROUND_WARNING = "#17130B";

const CARD_ACCENT = "#2B2B2B";
const CARD_BACKGROUND = "#121619";

function FaultCard({
  fault,
  acknowledge,
}: {
  fault: Fault;
  acknowledge: () => void;
}): ReactElement {
  const background = fault.acknowledged
    ? CARD_BACKGROUND
    : fault.severity === Severity.Error.valueOf()
      ? CARD_BACKGROUND_ERROR
      : CARD_BACKGROUND_WARNING;
  const accent = fault.acknowledged
    ? CARD_ACCENT
    : fault.severity === Severity.Error.valueOf()
      ? CARD_ACCENT_ERROR
      : CARD_ACCENT_WARNING;

  return (
    <div
      style={{
        overflow: "hidden",
        display: "flex",
        flexDirection: "column",
        gap: "0",
        padding: "0px",
        color: "white",
        background,
        fontSize: "12px",
        border: `1px solid ${accent}`,
        cursor: fault.acknowledged ? undefined : "pointer",
        outline: "none",
      }}
      tabIndex={-1}
      onClick={acknowledge}
    >
      <div
        style={{
          display: "flex",
          flexDirection: "row",
          alignItems: "center",
          justifyContent: "space-between",
          padding: "6px 8px",
          gap: "8px",
          background: accent,
          width: "100%",
        }}
      >
        <span
          style={{
            fontSize: "14px",
            fontWeight: "bold",
            textAlign: "left",
          }}
        >
          {FAULT_TYPE[fault.type]}
        </span>
        <div style={{ position: "relative", width: "12px", height: "12px" }}>
          <div
            style={{
              position: "absolute",
              inset: 0,
              opacity: 0.1,
              pointerEvents: "none",
              animation: "pulse 1s ease-out",
              borderRadius: "1000px",
            }}
          />
        </div>
      </div>
      <div
        style={{
          display: "flex",
          flexDirection: "column",
          gap: "10px",
          padding: "10px",
          userSelect: "none",
          outline: "none",
        }}
      >
        {Object.keys(fault.props).length > 0 ? (
          <div
            style={{
              display: "flex",
              flexDirection: "column",
              gap: "4px",
            }}
          >
            {fault.props.map(({ name, value }) => (
              <>
                <div
                  style={{
                    display: "flex",
                    flexDirection: "row",
                    alignItems: "center",
                    justifyContent: "space-between",
                    gap: "12px",
                    padding: "0px",
                    whiteSpace: "nowrap",
                  }}
                >
                  <span style={{ fontWeight: "bold", fontSize: "14px" }}>{value}</span>
                  <span style={{ fontSize: "12px" }}>{name}</span>
                </div>
              </>
            ))}
          </div>
        ) : (
          <></>
        )}
        <div
          style={{
            fontSize: "12px",
            textAlign: "left",
            width: "100%",
            // padding: "12px 12px",
            paddingTop: "0",
            whiteSpace: "nowrap",
          }}
        >
          {SEVERITY[fault.severity]}
          <br />
          Detected {date_to_str(fault.detected_at)}
        </div>
      </div>
    </div>
  );
}

function FaultsPanel({ context }: { context: PanelExtensionContext }): ReactElement {
  const [messages, setMessages] = useState<undefined | Immutable<MessageEvent[]>>();

  const [renderDone, setRenderDone] = useState<(() => void) | undefined>();

  const [activeFaults, setFaults] = useState<Fault[]>([]);
  const last_fault_n = useRef<number | null>(null);

  useLayoutEffect(() => {
    context.onRender = (renderState, done) => {
      setRenderDone(() => done);
      setMessages(renderState.currentFrame);
    };

    context.watch("topics");
    context.watch("currentFrame");
    context.subscribe([{ topic: "/CAN/MCU_OUT" }]);
  }, [context]);

  useEffect(() => {
    if (!messages || messages.length === 0) {
      return;
    }

    const lastMessage = messages[messages.length - 1]!;
    const message = lastMessage.message as CanMessage;

    let message_fault: MessageFault | null;
    if (message.fault_n !== 0 && message.fault_n !== last_fault_n.current) {
      message_fault = {
        type: message.fault_type,
        severity: message.fault_severity,
        arg: message.fault_arg,
        detected_at: message.fault_detected_at,
      };
    }
    last_fault_n.current = message.fault_n;

    setFaults((faults) => {
      if (message_fault) {
        const new_fault: Fault = {
          type: message_fault.type,
          severity: message_fault.severity,
          props: parse_arg(message_fault.type, message_fault.arg) ?? [],
          detected_at: new Date(message_fault.detected_at),
          acknowledged: false,
        };
        faults.unshift(new_fault);
      }

      return faults;
    });
  }, [messages]);

  // invoke the done callback once the render is complete
  useEffect(() => {
    renderDone?.();
  }, [renderDone]);

  return activeFaults.length > 0 ? (
    <>
      <div
        style={{
          display: "flex",
          flexDirection: "column",
          gap: "16px",
          padding: "8px",
          overflowY: "auto",
          height: "100%",
        }}
      >
        {activeFaults.length > 0 ? (
          <div
            style={{
              display: "flex",
              flexDirection: "column",
              gap: "8px",
              whiteSpace: "nowrap",
            }}
          >
            <div
              style={{
                width: "100%",
                display: "flex",
                justifyContent: "space-between",
                flexWrap: "wrap",
                columnGap: "6px",
                rowGap: "12px",
                alignItems: "center",
                color: "#8D939A",
              }}
            >
              <div
                style={{
                  display: "flex",
                  justifyContent: "space-between",
                  flexWrap: "wrap",
                  gap: "2px",
                  alignItems: "center",
                }}
              >
                <button
                  style={{
                    border: "1px solid #8D939A",
                    background: "transparent",
                    padding: "6px 12px",
                    cursor: "pointer",
                    color: "#8D939A",
                    outline: "none",
                  }}
                  onClick={() => {
                    setFaults((faults) => {
                      for (const fault of faults) {
                        fault.acknowledged = true;
                      }
                      return faults;
                    });
                  }}
                >
                  Acknowledge All
                </button>
                <button
                  style={{
                    border: "none",
                    background: "transparent",
                    padding: "6px 12px",
                    cursor: "pointer",
                    color: "#8D939A",
                    outline: "none",
                  }}
                  onClick={() => {
                    setFaults(() => {
                      return [];
                    });
                  }}
                >
                  Clear
                </button>
              </div>
              <span>{activeFaults.length} faults</span>
            </div>
            {activeFaults.map((fault, index) => (
              <FaultCard
                key={activeFaults.length - index}
                fault={fault}
                acknowledge={() => {
                  setFaults((faults) => {
                    faults[index]!.acknowledged = true;
                    return faults;
                  });
                }}
              />
            ))}
          </div>
        ) : (
          <></>
        )}
      </div>
    </>
  ) : (
    <>
      <div
        style={{
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          justifyContent: "center",
          width: "100%",
          height: "100%",
        }}
      >
        <span style={{ opacity: 0.8 }}>No faults</span>
      </div>
    </>
  );
}

export function initFaultsPanel(context: PanelExtensionContext): () => void {
  const root = createRoot(context.panelElement);
  root.render(<FaultsPanel context={context} />);

  // Return a function to run when the panel is removed
  return () => {
    root.unmount();
  };
}
