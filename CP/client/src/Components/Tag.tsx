import React from "react";

interface TagProps {
  status: string;
}

const statusColors: Record<string, string> = {
  Pending: "bg-amber-500 text-white",
  "In Progress": "bg-blue-500 text-white",
  Completed: "bg-green-500 text-white",
};

const Tag: React.FC<TagProps> = ({ status }) => {
  const colorClass = statusColors[status] || "bg-gray-500 text-white";

  if (!status) {
    status = "Undefined";
  }

  return (
    <span className={`px-2 py-1 text-xs font-semibold rounded-full whitespace-nowrap ${colorClass}`}>{status}</span>
  );
};

export default Tag;
